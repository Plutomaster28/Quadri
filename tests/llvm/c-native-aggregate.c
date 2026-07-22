typedef struct {
  long first;
  long second;
} SeaBirdPair;

typedef struct {
  float first;
  float second;
} SeaBirdFloatPair;

__attribute__((noinline)) SeaBirdPair seabird_pair_make(long first,
                                                        long second) {
  SeaBirdPair result = {first, second};
  return result;
}

__attribute__((noinline)) long seabird_pair_sum(SeaBirdPair value) {
  return value.first + value.second;
}

long seabird_pair_call(void) {
  volatile long first = 17;
  volatile long second = 25;
  return seabird_pair_sum(seabird_pair_make(first, second));
}

__attribute__((noinline)) SeaBirdFloatPair
seabird_float_pair_make(float first, float second) {
  SeaBirdFloatPair result = {first, second};
  return result;
}

__attribute__((noinline)) float
seabird_float_pair_sum(SeaBirdFloatPair value) {
  return value.first + value.second;
}

float seabird_float_pair_call(void) {
  volatile float first = 1.5f;
  volatile float second = 2.25f;
  return seabird_float_pair_sum(seabird_float_pair_make(first, second));
}
