function main () {
  numChannels = 2
  numSamples = 100
  channel1 = new Float32Array(numSamples)
  channel2 = new Float32Array(numSamples)
  data = new Float32Array(channel1.length + channel2.length)
  data.set(channel1)
  data.set(channel2, channel1.length)

  dataBytes = data.BYTES_PER_ELEMENT
  dataPtr = Module._malloc(numSamples * numChannels * dataBytes)
  dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numSamples * numChannels * dataBytes)
  dataHeap.set( new Uint8Array(data) )

  pointers = new Uint32Array(2)
  pointers[0] = dataPtr + 0 * data.BYTES_PER_ELEMENT * numSamples
  pointers[1] = dataPtr + 1 * data.BYTES_PER_ELEMENT * numSamples
  pointerBytes = pointers.length * pointers.BYTES_PER_ELEMENT
  pointerPtr = Module._malloc(pointerBytes)
  pointerHeap = new Uint8Array(Module.HEAPU8.buffer, pointerPtr, pointerBytes)
  pointerHeap.set( new Uint8Array(pointers.buffer) )

  read_wavpack_file = Module.cwrap('read_wavpack_file', 'number', ['string', 'string', 'number'])
  inPath = "assets/007cbe4e63c592a6-ffffffffff5a0601_audio_compressedRaw"
  outPath = "assets/007cbe4e63c592a6-ffffffffff5a0601_audio_compressedRawCorrection"

  result = read_wavpack_file(inPath, outPath, pointerHeap.byteOffset, numSamples)
  console.log(pointerHeap.byteOffset)
  newChannel1 = Module.HEAPF32.slice(
    pointers[0] / Float32Array.BYTES_PER_ELEMENT,
    pointers[0] / Float32Array.BYTES_PER_ELEMENT + numSamples
  )
  newChannel2 = Module.HEAPF32.slice(
    pointers[1] / Float32Array.BYTES_PER_ELEMENT,
    pointers[1] / Float32Array.BYTES_PER_ELEMENT + numSamples
  )
  console.log(newChannel1, newChannel2)
}
