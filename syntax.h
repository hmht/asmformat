#include<stdbool.h>
/*@unused@*/ extern bool is_allcaps(char const*token);
/*@unused@*/ extern bool is_newline(char const*token);
/*@unused@*/ extern bool is_comment(char const*token);
/*@unused@*/ extern bool is_macro(char const*token);
/*@unused@*/ extern bool is_segment_label(char const*token);
/*@unused@*/ extern bool is_addressable_declaration(char const*token);
/*@unused@*/ extern bool is_declaration(char const*token);
/*@unused@*/ extern bool is_mnemonic(char const*token);
/*@unused@*/ extern bool is_mnemonic_or_declaration(char const*token);
/*@unused@*/ extern bool is_word_char(bool first, char c);
/*@unused@*/ extern bool has_blob(char const*token);
/*@unused@*/ extern bool has_arguments(char const*token);
/*@unused@*/ extern bool opens_string(char c);
/*@unused@*/ extern bool closes_string(char prev, char next);
