// swift-tools-version: 5.5
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "WavpackFileReader",
    platforms: [.iOS(.v11)],
    products: [
        .library(name: "WavpackFileReader", targets: ["WavpackFileReader"]),
    ],
    dependencies: [],
    targets: [
        .target(name: "WavpackFileReader", dependencies: ["CWavpackFileReader"]),
        .target(name: "CWavpackFileReader", cxxSettings: [.headerSearchPath("."), .define("ENABLE_THREADS", to: "1")]),
        .testTarget(name: "WavpackFileReaderTests", dependencies: ["WavpackFileReader"], resources: [.copy("TestResources")]),
    ],
    cxxLanguageStandard: .cxx14
)
