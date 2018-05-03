import qbs

Project {
    minimumQbsVersion: "1.6.0"

    CppApplication {
        consoleApplication: true
        files: [
            "main.cpp",
        ]

        cpp.cxxFlags: [
            "-Wno-unused-variable",
            "-Wno-unused-function",
            "-Wno-strict-aliasing",

            "-fno-exceptions",
            "-fno-rtti"
        ]
    }
}
