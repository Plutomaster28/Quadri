target triple = "seabird32-unknown-none"

define i32 @tritium_atomic_add(ptr %address, i32 %value) {
entry:
  %old = atomicrmw add ptr %address, i32 %value acq_rel
  ret i32 %old
}

define i32 @tritium_atomic_sub(ptr %address, i32 %value) {
entry:
  %old = atomicrmw sub ptr %address, i32 %value acq_rel
  ret i32 %old
}

define i32 @tritium_atomic_and(ptr %address, i32 %value) {
entry:
  %old = atomicrmw and ptr %address, i32 %value acq_rel
  ret i32 %old
}

define i32 @tritium_atomic_or(ptr %address, i32 %value) {
entry:
  %old = atomicrmw or ptr %address, i32 %value acq_rel
  ret i32 %old
}

define i32 @tritium_atomic_xor(ptr %address, i32 %value) {
entry:
  %old = atomicrmw xor ptr %address, i32 %value acq_rel
  ret i32 %old
}

define i32 @tritium_cmpxchg_old(ptr %address, i32 %expected, i32 %desired) {
entry:
  %pair = cmpxchg ptr %address, i32 %expected, i32 %desired acq_rel acquire
  %old = extractvalue { i32, i1 } %pair, 0
  ret i32 %old
}

define i1 @tritium_cmpxchg_success(ptr %address, i32 %expected, i32 %desired) {
entry:
  %pair = cmpxchg ptr %address, i32 %expected, i32 %desired acq_rel acquire
  %success = extractvalue { i32, i1 } %pair, 1
  ret i1 %success
}

define i32 @tritium_atomic_load(ptr %address) {
entry:
  %value = load atomic i32, ptr %address acquire, align 4
  ret i32 %value
}

define void @tritium_atomic_store(ptr %address, i32 %value) {
entry:
  store atomic i32 %value, ptr %address release, align 4
  ret void
}

define void @tritium_fence() {
entry:
  fence seq_cst
  ret void
}
