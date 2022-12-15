import CWavpackFileReader
import Foundation
import AVFoundation

public class WavpackFileWriter {

    public let format: AVAudioFormat

    private let handle: wavpack_file_handle_t

    public init(wvURL: URL, wvcURL: URL) throws {
        guard let wvPath = wvURL.path.cString(using: .utf8) else {
            throw WavpackFileReaderError.invalidPath("Failed to convert .wv file path to C string")
        }
        guard let wvcPath = wvcURL.path.cString(using: .utf8) else {
            throw WavpackFileReaderError.invalidPath("Failed to convert .wvc file path to C string")
        }
        var handle: wavpack_file_handle_t?
        let result = wavpack_file_open_for_writing(wvPath, wvcPath, 2, 16, 48000, &handle)
        guard result.isSuccess else {
            throw result.toWavpackFileReaderError()
        }
        guard let handle = handle else {
            fatalError("Did not get handle back from successful open")
        }
        let numChannels = AVAudioChannelCount(wavpack_file_get_num_channels(handle))
        let sampleRate = Double(wavpack_file_get_sample_rate(handle))
        guard let format = AVAudioFormat(standardFormatWithSampleRate: sampleRate, channels: numChannels) else {
            throw WavpackFileReaderError.invalidFormat
        }
        self.handle = handle
        self.format = format
    }

    deinit {
        wavpack_file_close(handle)
    }

}
