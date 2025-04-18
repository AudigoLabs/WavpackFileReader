async function main () {
  const read_wavpack_file = Module.cwrap('read_wavpack_file', 'number', ['string', 'string', 'number'])
  const read_wavpack_buffer = Module.cwrap('read_wavpack_buffer', 'number', ['number', 'number', 'number', 'number', 'number'])

  const numChannels = 2
  const numSamples = 100
  const wvPath = "assets/007cbe4e63c592a6-ffffffffff5a0601_audio_compressedRaw"
  const wvcPath = "assets/007cbe4e63c592a6-ffffffffff5a0601_audio_compressedRawCorrection"

  // {
  //   // Construct a buffer for each channel
  //   const channel1 = new Float32Array(numSamples)
  //   const channel2 = new Float32Array(numSamples)
  //   const data = new Float32Array(channel1.length + channel2.length)
  //   data.set(channel1)
  //   data.set(channel2, channel1.length)
  
  //   // Concatenate the two buffers into a single buffer
  //   const dataBytes = data.BYTES_PER_ELEMENT
  //   const dataPtr = Module._malloc(numSamples * numChannels * dataBytes)
  //   const dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numSamples * numChannels * dataBytes)
  //   dataHeap.set( new Uint8Array(data) )
  
  //   // Create a two-item array that points to our channel buffers
  //   const pointers = new Uint32Array(2)
  //   pointers[0] = dataPtr + 0 * data.BYTES_PER_ELEMENT * numSamples
  //   pointers[1] = dataPtr + 1 * data.BYTES_PER_ELEMENT * numSamples
  //   const pointerBytes = pointers.length * pointers.BYTES_PER_ELEMENT
  //   const pointerPtr = Module._malloc(pointerBytes)
  //   const pointerHeap = new Uint8Array(Module.HEAPU8.buffer, pointerPtr, pointerBytes)
  //   pointerHeap.set( new Uint8Array(pointers.buffer) )
  //   console.log(pointerHeap.byteOffset)
  
  //   // Load data out of the given paths into the structure we just built
  //   const result = read_wavpack_file(wvPath, wvcPath, pointerHeap.byteOffset, numSamples)
  //   console.log('result', result)
  //   const newChannel1 = Module.HEAPF32.slice(
  //     pointers[0] / Float32Array.BYTES_PER_ELEMENT,
  //     pointers[0] / Float32Array.BYTES_PER_ELEMENT + numSamples
  //   )
  //   const newChannel2 = Module.HEAPF32.slice(
  //     pointers[1] / Float32Array.BYTES_PER_ELEMENT,
  //     pointers[1] / Float32Array.BYTES_PER_ELEMENT + numSamples
  //   )
  //   console.log(newChannel1, newChannel2)
  // }

  {
    // Construct an input buffer for each channel
    const blob1 = await fetch(wvPath)
    .then((res) => res.blob())
    .then((blob) => blob.arrayBuffer())
    .then((arr) => new Uint8Array(arr))
    // .then((arr) => Uint8Array.from(arr))
    const blob2 = await fetch(wvcPath)
    .then((res) => res.blob())
    .then((blob) => blob.arrayBuffer())
    .then((arr) => new Uint8Array(arr))
    // .then((arr) => Uint8Array.from(arr))
    console.log(blob1, blob2)
  
    const blob1Bytes = blob1.BYTES_PER_ELEMENT
    const blob1Ptr = Module._malloc(numSamples * numChannels * blob1Bytes)
    const blob1Heap = new Uint8Array(Module.HEAPU8.buffer, blob1Ptr, blob1.byteLength)
    blob1Heap.set( new Uint8Array(blob1) )
  
    const blob2Bytes = blob2.BYTES_PER_ELEMENT
    const blob2Ptr = Module._malloc(numSamples * numChannels * blob2Bytes)
    const blob2Heap = new Uint8Array(Module.HEAPU8.buffer, blob2Ptr, blob2.byteLength)
    blob2Heap.set( new Uint8Array(blob2) )
    
    // Construct an output buffer for each channel
    const channel1 = new Float32Array(numSamples)
    const channel2 = new Float32Array(numSamples)
    const data = new Float32Array(channel1.length + channel2.length)
    data.set(channel1)
    data.set(channel2, channel1.length)
  
    // Concatenate the two buffers into a single buffer
    const dataBytes = data.BYTES_PER_ELEMENT
    const dataPtr = Module._malloc(numSamples * numChannels * dataBytes)
    const dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numSamples * numChannels * dataBytes)
    dataHeap.set( new Uint8Array(data) )
  
    // Create a two-item array that points to our channel buffers
    const pointers = new Uint32Array(2)
    pointers[0] = dataPtr + 0 * data.BYTES_PER_ELEMENT * numSamples
    pointers[1] = dataPtr + 1 * data.BYTES_PER_ELEMENT * numSamples
    const pointerBytes = pointers.length * pointers.BYTES_PER_ELEMENT
    const pointerPtr = Module._malloc(pointerBytes)
    const pointerHeap = new Uint8Array(Module.HEAPU8.buffer, pointerPtr, pointerBytes)
    pointerHeap.set( new Uint8Array(pointers.buffer) )
    console.log(pointerHeap.byteOffset)
  
    // Load data out of the given paths into the structure we just built
    const result = read_wavpack_buffer(
      blob1Heap.byteOffset,
      blob1.byteLength,
      blob2Heap.byteOffset,
      blob2.byteLength,
      pointerHeap.byteOffset
    )
    console.log('result', result)
    const newChannel1 = Module.HEAPF32.slice(
      pointers[0] / Float32Array.BYTES_PER_ELEMENT,
      pointers[0] / Float32Array.BYTES_PER_ELEMENT + numSamples
    )
    const newChannel2 = Module.HEAPF32.slice(
      pointers[1] / Float32Array.BYTES_PER_ELEMENT,
      pointers[1] / Float32Array.BYTES_PER_ELEMENT + numSamples
    )
    console.log(newChannel1, newChannel2)
  }
}
