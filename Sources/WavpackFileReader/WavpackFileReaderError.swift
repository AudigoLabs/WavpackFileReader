import CWavpackFileReader
import Foundation

public enum WavpackFileReaderError : Error {

    case invalidPath(String)
    case invalidFormat

    case openFailed
    case fileError
    case invalidParam
    case unknownCError(UInt32)

}

extension wavpack_file_result_t {

    var isSuccess: Bool { self == WAVPACK_FILE_RESULT_SUCCESS }

    func toWavpackFileReaderError() -> WavpackFileReaderError {
        switch self {
        case WAVPACK_FILE_RESULT_OPEN_FAILED:
            return .openFailed
        case WAVPACK_FILE_RESULT_FILE_ERROR:
            return .fileError
        case WAVPACK_FILE_RESULT_INVALID_PARAM:
            return .invalidParam
        default:
            return .unknownCError(rawValue)
        }
    }

}
