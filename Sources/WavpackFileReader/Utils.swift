//
//  Utils.swift
//  WavpackFileReader
//
//  Created by Brian Gomberg on 6/26/25.
//

import CWavpackFileReader
import Foundation

extension URL {

    var cStringPath: [CChar]? {
        path.cString(using: .utf8)
    }

}

extension wavpack_file_result_t {

    func checkSuccess() throws(WavpackFileReaderError) {
        switch self {
        case WAVPACK_FILE_RESULT_SUCCESS: break
        case WAVPACK_FILE_RESULT_OPEN_FAILED: throw .openFailed
        case WAVPACK_FILE_RESULT_FILE_ERROR: throw .fileError
        case WAVPACK_FILE_RESULT_INVALID_PARAM: throw .invalidParam
        default: throw .unknownCError(rawValue)
        }
    }

}
