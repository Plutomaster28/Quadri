@local_data = global i64 41, align 8
@external_data = external global i64

define ptr @address_of_local() {
entry:
  ret ptr @local_data
}

define ptr @address_of_external() {
entry:
  ret ptr @external_data
}

define i64 @load_local() {
entry:
  %value = load i64, ptr @local_data, align 8
  ret i64 %value
}

define void @store_local(i64 %value) {
entry:
  store i64 %value, ptr @local_data, align 8
  ret void
}
