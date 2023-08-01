import CWavpackFileReader
import Foundation
import AVFoundation

public class WavpackBufferReader {

    public let format: AVAudioFormat

    private let handle: wavpack_file_handle_t

    public init(wvData: Data, wvcData: Data?) throws {
        var handle: wavpack_file_handle_t?
        let result: wavpack_file_result_t = wvData.withUnsafeBytes { wvPtr in
            if let wvcData {
                return wvcData.withUnsafeBytes { wvcPtr in
                    return wavpack_file_open_raw(wvPtr.baseAddress!, Int32(wvData.count), wvcPtr.baseAddress!, Int32(wvcData.count), &handle)
                }
            }
            else {
                return wavpack_file_open_raw(wvPtr.baseAddress!, Int32(wvData.count), nil, 0, &handle)
            }
        }
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

    public var blockOffset: UInt32 {
        wavpack_file_get_raw_block_offset(handle)
    }

    public var duration: TimeInterval {
        wavpack_file_get_duration(handle)
    }

    public func readFrames() -> AVAudioPCMBuffer {
        guard let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: AVAudioFrameCount(wavpack_file_get_num_samples(handle))),
              let floatChannelData = buffer.floatChannelData else {
            fatalError("Failed to create buffer")
        }
        buffer.frameLength = wavpack_file_read(handle, floatChannelData, buffer.frameCapacity)
        return buffer
    }

}
