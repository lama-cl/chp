C6d1b expected behavior:

- The call under [tag] transfers consume_int_box.$0.payload cast histories and
  pointer operations to run::box.payload and run::x as int*.
- The call under [!(tag)] transfers consume_char_box.$0.payload cast histories
  and pointer operations to run::box.payload and run::x as char*.
- Branch guards should be preserved on both the transferred cast-history
  observations and the transferred data-field pointer operations.
