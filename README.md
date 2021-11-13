# SimdClauJson2
Using simdjson/simdjson (https://github.com/simdjson/simdjson) Apache-2.0 License

# Changed  for simdjson/simdjson
  ```c++
    inline const std::unique_ptr<uint64_t[]>& parser::raw_tape() const noexcept {
        return doc.tape;
    }

    inline const std::unique_ptr<uint8_t[]>& parser::raw_string_buf() const noexcept {
        return doc.string_buf;
    }
```
