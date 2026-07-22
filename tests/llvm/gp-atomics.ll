target triple = "seabird64-unknown-none"

define i64 @seabird_atomic_exchange(ptr %address, i64 %value) {
entry:
  %old = atomicrmw xchg ptr %address, i64 %value acq_rel
  ret i64 %old
}
