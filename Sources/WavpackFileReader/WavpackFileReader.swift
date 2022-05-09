import CWavpackFileReader
import Foundation
import AVFoundation

public class WavpackFileReader {

    public let format: AVAudioFormat

    private let handle: wavpack_file_handle_t

    public init(wvURL: URL, wvcURL: URL?) throws {
        guard let wvPath = wvURL.path.cString(using: .utf8) else {
            throw WavpackFileReaderError.invalidPath("Failed to convert .wv file path to C string")
        }
        var wvcPath: [CChar]? = nil
        if let wvcURL = wvcURL {
            wvcPath = wvcURL.path.cString(using: .utf8)
            guard wvcPath != nil else {
                throw WavpackFileReaderError.invalidPath("Failed to convert .wvc file path to C string")
            }
        }
        var handle: wavpack_file_handle_t?
        let result = wavpack_file_open(wvPath, wvcPath, &handle)
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

    public var duration: TimeInterval {
        wavpack_file_get_duration(handle)
    }

    public var fileBitsPerSample: UInt16 {
        wavpack_file_get_bits_per_sample(handle)
    }

    public func readFrames(frameCapacity: UInt32) -> AVAudioPCMBuffer {
        guard let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: frameCapacity),
              let floatChannelData = buffer.floatChannelData else {
            fatalError("Failed to create buffer")
        }
        buffer.frameLength = wavpack_file_read(handle, floatChannelData, buffer.frameCapacity)
        return buffer
    }

    public func seek(position: TimeInterval) throws {
        let result = wavpack_file_set_seek(handle, position)
        guard result.isSuccess else {
            throw result.toWavpackFileReaderError()
        }
    }

    public func setOffset(_ offset: UInt32) throws {
        let result = wavpack_file_set_offset(handle, offset)
        guard result.isSuccess else {
            throw result.toWavpackFileReaderError()
        }
    }

}
