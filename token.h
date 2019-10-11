#include<stdbool.h>
struct syntax {
	/** the given token expects following comma-separated tokens */
	bool (*has_arguments)(char const*);
	/** the given token expects a blob you want to parse manually (including comment) */
	bool (*has_blob)(char const*);
	/** the given token is a comment */
	bool (*is_comment)(char const*);
	bool (*is_word_char)(bool, char);
	/** this name is wrong, and should not return true when the given token is a macro.
	 * make this return false unless you have no other way to handle a token*/
	bool (*is_macro)(char const*);
	bool (*opens_string)(char);
	bool (*closes_string)(char,char);
};

extern bool split_line_into_parts(struct syntax const*syntax, char const*line, char***tokens);

extern void free_token_list(char**tokens);
