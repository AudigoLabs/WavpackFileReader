import WavpackFileReader
import XCTest
import AVFAudio

final class WavFileReaderTests: XCTestCase {

    func testProperties() throws {
        let wvURL = Bundle.module.url(forResource: "drum", withExtension: "wv", subdirectory: "TestResources")!
        let wvcURL = Bundle.module.url(forResource: "drum", withExtension: "wvc", subdirectory: "TestResources")!
        let reader = try WavpackFileReader(wvURL: wvURL, wvcURL: wvcURL)
        XCTAssertEqual(reader.format.channelCount, 2)
        XCTAssertEqual(reader.format.sampleRate, 48000)
        XCTAssertEqual(reader.duration, 45.28)
        XCTAssertEqual(reader.fileBitsPerSample, 16)
    }

    func testReadWV() throws {
        // Open the file using our reader
        let wvURL = Bundle.module.url(forResource: "drum", withExtension: "wv", subdirectory: "TestResources")!
        let reader = try WavpackFileReader(wvURL: wvURL, wvcURL: nil)

        // Compare our reader with AVAudioFile
        let wavURL = Bundle.module.url(forResource: "drum", withExtension: "wav", subdirectory: "TestResources")!
        let audioFile = try AVAudioFile(forReading: wavURL)

        // Compare the number of frames
        let expectedNumFrames = AVAudioFrameCount(audioFile.length)
        XCTAssertEqual(Double(expectedNumFrames) / audioFile.fileFormat.sampleRate, 45.28)

        // Read the expected file
        let expectedBuffer = AVAudioPCMBuffer(pcmFormat: reader.format, frameCapacity: expectedNumFrames)!
        try audioFile.read(into: expectedBuffer)

        // Read the entire file using our reader
        let buffer = reader.readFrames(frameCapacity: expectedNumFrames)
        XCTAssertEqual(buffer.frameLength, expectedNumFrames)
    }

    func testReadWVC() throws {
        // Open the file using our reader
        let wvURL = Bundle.module.url(forResource: "drum", withExtension: "wv", subdirectory: "TestResources")!
        let wvcURL = Bundle.module.url(forResource: "drum", withExtension: "wvc", subdirectory: "TestResources")!
        let reader = try WavpackFileReader(wvURL: wvURL, wvcURL: wvcURL)

        // Compare our reader with AVAudioFile
        let wavURL = Bundle.module.url(forResource: "drum", withExtension: "wav", subdirectory: "TestResources")!
        let audioFile = try AVAudioFile(forReading: wavURL)

        // Compare the number of frames
        let expectedNumFrames = AVAudioFrameCount(audioFile.length)
        XCTAssertEqual(Double(expectedNumFrames) / audioFile.fileFormat.sampleRate, 45.28)

        // Read the expected file
        let expectedBuffer = AVAudioPCMBuffer(pcmFormat: reader.format, frameCapacity: expectedNumFrames)!
        try audioFile.read(into: expectedBuffer)

        // Read the entire file using our reader
        let buffer = reader.readFrames(frameCapacity: expectedNumFrames)
        XCTAssertEqual(buffer.frameLength, expectedNumFrames)

        // Compare each channel
        for i in 0..<Int(reader.format.channelCount) {
            XCTAssertEqual(memcmp(buffer.floatChannelData![i], expectedBuffer.floatChannelData![i], Int(expectedNumFrames) * MemoryLayout<Float>.stride), 0)
        }
    }

    func testSeekAndRead() throws {
        // Open the file using our reader
        let wvURL = Bundle.module.url(forResource: "drum", withExtension: "wv", subdirectory: "TestResources")!
        let wvcURL = Bundle.module.url(forResource: "drum", withExtension: "wvc", subdirectory: "TestResources")!
        let reader = try WavpackFileReader(wvURL: wvURL, wvcURL: wvcURL)

        // Compare our reader with AVAudioFile
        let wavURL = Bundle.module.url(forResource: "drum", withExtension: "wav", subdirectory: "TestResources")!
        let audioFile = try AVAudioFile(forReading: wavURL)
        audioFile.framePosition = Int64(0.6 * reader.format.sampleRate + 0.5)

        // Read the expected buffer
        let expectedBuffer = AVAudioPCMBuffer(pcmFormat: reader.format, frameCapacity: 100)!
        try audioFile.read(into: expectedBuffer)

        // Read the buffer using our reader
        try reader.seek(position: 0.6)
        let buffer = reader.readFrames(frameCapacity: 100)
        XCTAssertEqual(buffer.frameLength, 100)
    }

    func testWriteRead() throws {
        // Open and read the uncompressed file
        let wavURL = Bundle.module.url(forResource: "drum", withExtension: "wav", subdirectory: "TestResources")!
        let wavFile = try AVAudioFile(forReading: wavURL)
        let wavBuffer = AVAudioPCMBuffer(pcmFormat: wavFile.processingFormat, frameCapacity: UInt32(wavFile.length))!
        try wavFile.read(into: wavBuffer)

        // Write the file
        let tempDirURL = URL(fileURLWithPath: NSTemporaryDirectory(), isDirectory: true)
        let wvURL = tempDirURL.appendingPathComponent("test.wv")
        let wvcURL = tempDirURL.appendingPathComponent("test.wvc")

        try {
            let writer = try WavpackFileWriter(wvURL: wvURL, wvcURL: wvcURL, bitsPerSample: 24, numChannels: UInt16(wavBuffer.format.channelCount), sampleRate: UInt32(wavBuffer.format.sampleRate))
            try writer.writeFrames(wavBuffer)
        }()

        defer {
            try? FileManager.default.removeItem(at: wvURL)
            try? FileManager.default.removeItem(at: wvcURL)
        }

        // Open the newly-written file using our reader
        let reader = try WavpackFileReader(wvURL: wvURL, wvcURL: wvcURL)

        // Compare the number of frames
        let expectedNumFrames = AVAudioFrameCount(wavBuffer.frameLength)
        XCTAssertEqual(Double(expectedNumFrames) / wavFile.fileFormat.sampleRate, 45.28)

        // Read the entire file using our reader
        let buffer = reader.readFrames(frameCapacity: expectedNumFrames)
        XCTAssertEqual(buffer.frameLength, expectedNumFrames)

        // Compare each channel
        for i in 0..<Int(reader.format.channelCount) {
            XCTAssertEqual(memcmp(buffer.floatChannelData![i], wavBuffer.floatChannelData![i], Int(expectedNumFrames) * MemoryLayout<Float>.stride), 0)
        }
    }

}
