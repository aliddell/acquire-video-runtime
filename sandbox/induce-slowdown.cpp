#include "acquire.h"
#include "device/hal/device.manager.h"
#include "platform.h"
#include "logger.h"

#include <cstdio>
#include <stdexcept>

void
reporter(int is_error,
         const char* file,
         int line,
         const char* function,
         const char* msg)
{
    fprintf(is_error ? stderr : stdout,
            "%s%s(%d) - %s: %s\n",
            is_error ? "ERROR " : "",
            file,
            line,
            function,
            msg);
}

/// Helper for passing size static strings as function args.
/// For a function: `f(char*,size_t)` use `f(SIZED("hello"))`.
/// Expands to `f("hello",5)`.
#define SIZED(str) str, sizeof(str)

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            char buf[1 << 8] = { 0 };                                          \
            ERR(__VA_ARGS__);                                                  \
            snprintf(buf, sizeof(buf) - 1, __VA_ARGS__);                       \
            throw std::runtime_error(buf);                                     \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false: %s", #e)
#define DEVOK(e) CHECK(Device_Ok == (e))
#define OK(e) CHECK(AcquireStatus_Ok == (e))

void
setup(AcquireRuntime* runtime)
{
    CHECK(runtime);

    auto dm = acquire_device_manager(runtime);
    CHECK(dm);

    AcquireProperties props = {};
    OK(acquire_get_configuration(runtime, &props));

    DEVOK(device_manager_select(dm,
                                DeviceKind_Camera,
                                SIZED("simulated.*radial.*") - 1,
                                &props.video[0].camera.identifier));
    DEVOK(device_manager_select(dm,
                                DeviceKind_Storage,
                                SIZED("trash") - 1,
                                &props.video[0].storage.identifier));

    storage_properties_init(
      &props.video[0].storage.settings, 0, nullptr, 0, nullptr, 0, { 0 });

    OK(acquire_configure(runtime, &props));

    AcquirePropertyMetadata metadata = { 0 };
    OK(acquire_get_configuration_metadata(runtime, &metadata));

    props.video[0].camera.settings.binning = 1;
    props.video[0].camera.settings.pixel_type = SampleType_u16;
    props.video[0].camera.settings.shape = {
        .x = 1920,
        .y = 1080,
    };
    props.video[0].camera.settings.exposure_time_us = 1e2;
    props.video[0].max_frame_count = 100;

    OK(acquire_configure(runtime, &props));
}

void
acquire(AcquireRuntime* runtime)
{
    struct clock clock
    {};
    // expected time to acquire frames + 100%
    static double time_limit_ms = 20000.0;
    clock_init(&clock);
    clock_shift_ms(&clock, time_limit_ms);

    OK(acquire_start(runtime));
    {
        while (acquire_get_state(runtime) == DeviceState_Running) {
            struct clock throttle = {};
            clock_init(&throttle);
            EXPECT(clock_cmp_now(&clock) < 0,
                   "Timeout at %f ms",
                   clock_toc_ms(&clock) + time_limit_ms);
            clock_sleep_ms(&throttle, 100.0f);
        }
        OK(acquire_stop(runtime));
    }
}

int
main()
{
    auto runtime = acquire_init(reporter);

    setup(runtime);
    acquire(runtime);

    OK(acquire_shutdown(runtime));

    return 0;
}
