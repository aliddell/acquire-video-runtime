if(${WITH_SANDBOX})
    set(NOTEST "TRUE")
    aq_require(acquire-driver-common)
    set(NOTEST "FALSE")

    #
    # PARAMETERS
    #
    set(project acquire-driver-runtime) # CMAKE_PROJECT_NAME gets overridden if this is a subtree of another project
    set(driver acquire-driver-common)

    #
    # Tests
    #
    set(examples
        induce-slowdown
    )

    foreach(name ${examples})
        set(tgt "${project}-${name}")
        add_executable(${tgt} ${name}.cpp)
        target_link_libraries(${tgt}
            acquire-video-runtime
            acquire-device-kit
            acquire-device-hal
        )
        target_compile_definitions(${tgt} PUBLIC TEST="${tgt}")
        add_test(NAME test-${tgt} COMMAND ${tgt})
        set_tests_properties(test-${tgt} PROPERTIES LABELS "anyplatform;acquire-video-runtime")
    endforeach()

    #
    # Copy driver to examples
    #
    list(GET examples 0 onename)
    add_custom_target(${project}-copy-${driver}-for-sandbox
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${driver}>
        $<TARGET_FILE_DIR:${project}-${onename}>
        DEPENDS ${driver}
        COMMENT "Copying ${driver} to $<TARGET_FILE_DIR:${project}-${onename}>"
    )

    foreach(name ${examples})
        add_dependencies(${tgt} ${project}-copy-${driver}-for-sandbox)
    endforeach()
endif()
