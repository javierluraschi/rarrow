#include <arrow/api.h>
#include <arrow/buffer.h>
#include "Rcpp.h"
#include "arrow_types.h"

using namespace arrow;
using namespace Rcpp;

class RBuffer : public Buffer {
 public:
  RBuffer(SEXP data) : Buffer(nullptr, 0) {
    is_mutable_ = false;
    data_ = reinterpret_cast<const uint8_t*>(INTEGER(data));
    size_ = Rf_length(data) * sizeof(int);
    capacity_ = size_;
  }
  ~RBuffer() {}
};

std::shared_ptr<Buffer> values_to_bitmap(IntegerVector array) {
  std::vector<int32_t> values = as<std::vector<int32_t> >(array);
  auto bitmap = MutableBuffer::Wrap(values.data(), values.size());
  auto bitmap_data = bitmap->mutable_data();

  for (int i = 0; i < array.size(); ++i) {
    if (array.at(i) != NA_INTEGER) {
      BitUtil::SetBit(bitmap_data, i);
    }
  }

  return bitmap;
}

// [[Rcpp::export]]
array_ptr array(IntegerVector input) {
  auto buffer = std::make_shared<RBuffer>(input);

  // The first buffer is the null bitmap, which we are ignoring for now
  BufferVector buffers = {values_to_bitmap(input), buffer};

  auto data = std::make_shared<ArrayData>(int32(), input.length(),
                                          std::move(buffers), 0, 0);
  return array_ptr(new std::shared_ptr<Array>(MakeArray(data)));
}

// [[Rcpp::export]]
CharacterVector array_string(array_ptr const& array) {
  std::shared_ptr<arrow::Array> p = *array;
  return p->ToString();
}

// [[Rcpp::export]]
IntegerVector as_r_int(array_ptr const& array) {
  std::shared_ptr<Int32Array> int32_array =
      std::static_pointer_cast<Int32Array>(*array);

  int n = (*array)->length();

  IntegerVector out(n);
  memcpy(INTEGER(out), int32_array->raw_values(), n * sizeof(int));

  return out;
}
