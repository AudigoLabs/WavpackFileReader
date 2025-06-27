import CWavpackFileReader
import Foundation
import AVFoundation

public class WavpackFileWriter {

    public let format: AVAudioFormat

    private let handle: wavpack_file_handle_t
    private var isOpen = false

    public init(wvURL: URL, wvcURL: URL?, bitsPerSample: UInt16, numChannels: UInt16, sampleRate: UInt32) throws {
        guard let wvPath = wvURL.cStringPath else {
            throw WavpackFileReaderError.invalidPath("Failed to convert .wv file path to C string")
        }
        let wvcPath: [CChar]?
        if let wvcURL {
            guard let cStr = wvcURL.cStringPath else {
                throw WavpackFileReaderError.invalidPath("Failed to convert .wvc file path to C string")
            }
            wvcPath = cStr
        } else {
            wvcPath = nil
        }
        var config = wavpack_write_config_t(num_channels: numChannels, bits_per_sample: bitsPerSample, sample_rate: sampleRate)
        var handle: wavpack_file_handle_t?
        let result = wavpack_file_open_for_writing(&config, wvPath, wvcPath, &handle)
        try result.checkSuccess()
        guard let handle else {
            fatalError("Did not get handle back from successful open")
        }
        let numChannels = AVAudioChannelCount(wavpack_file_get_num_channels(handle))
        let sampleRate = Double(wavpack_file_get_sample_rate(handle))
        guard let format = AVAudioFormat(standardFormatWithSampleRate: sampleRate, channels: numChannels) else {
            throw WavpackFileReaderError.invalidFormat
        }
        self.handle = handle
        self.format = format
        isOpen = true
    }

    deinit {
        wavpack_file_close(handle)
    }

    public func writeFrames(_ buffer: AVAudioPCMBuffer) throws {
        guard let floatChannelData = buffer.floatChannelData else {
            throw WavpackFileReaderError.invalidFormat
        }
        try wavpack_file_write(handle, floatChannelData, buffer.frameLength).checkSuccess()
    }
}
