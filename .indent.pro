/* Derived from GNU Indent for the "-linux" option */
/* Divergence marked by keeping old setting in preceding comment */

--no-blank-lines-after-declarations
--blank-lines-after-procedures
--no-blank-lines-after-commas
--break-before-boolean-operator
--honour-newlines
--braces-on-if-line
--braces-on-struct-decl-line
// -- Can't be inhibited ?
--comment-indentation33
// -- Can't be inhibited ?
--declaration-comment-column33
--no-comment-delimiters-on-blank-lines
--cuddle-else
--continuation-indentation4

--case-indentation0
--line-comments-indentation0
--declaration-indentation1
--dont-format-first-column-comments
/* --indent-level8 */
--indent-level4
--parameter-indentation0
--line-length80
--continue-at-parentheses
--no-space-after-function-call-names
--no-space-after-parentheses
--dont-break-procedure-type
--space-after-if

--space-after-for
--space-after-while
--no-space-after-casts
--dont-star-comments
--swallow-optional-blank-lines
--dont-format-comments
// -- Can't be inhibited ?
--else-endif-column33
--space-special-semicolon
/* --tab-size8 */
--tab-size4
/* --indent-label1 */
--indent-label0

/* Additional Indent options */
--leave-preprocessor-space
--no-tabs
--preserve-mtime
--case-indentation4

/* Local types*/
-T LDATA
-T handle_t

/* Target-system specific types  */
-T FILE

/* C99 stdint.h types GNU indent seems unaware of */
-T int8_t
-T int16_t
-T int32_t
-T int64_t
-T uint8_t
-T uint16_t
-T uint32_t
-T uint64_t

/* POSIX types */
-T regex_t
-T regmatch_t
-T regoff_t
