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
