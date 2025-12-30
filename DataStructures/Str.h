/*!
   Str.h

    Created on: 10.02.2017
        Author: Sambulov Dmitry
        e-mail: dmitry.sambulov@gmail.com
 */
#ifndef STR_H_INCLUDED
#define STR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define IS_UPALPHA(A)   ((A) >= 'A' && (A) <= 'Z')
#define IS_LOALPHA(A)   ((A) >= 'a' && (A) <= 'z')
#define IS_DIGIT(A)     ((A) >= '0' && (A) <= '9')
#define IS_ALPHA(A)     (IS_UPALPHA(A) || IS_LOALPHA(A))
#define IS_HEX(A)       (IS_DIGIT(A) || ((A) >= 'A' && (A) <= 'F') || ((A) >= 'a' && (A) <= 'f'))
#define TO_LOWER(A)     ((IS_UPALPHA(A)) ? ( (A) - 'A' + 'a' ) : (A))
#define TO_UPPER(A)     ((IS_LOALPHA(A)) ? ( (A) - 'a' + 'A' ) : (A))
#define STR_LENGTH(A)   (sizeof(A) - 1)
#define CHAR_TO_HEX(A)  (IS_DIGIT(A)? ((A) - '0'): ((IS_UPALPHA(A)? ((A) - 'A'):((A) - 'a')) + 10))
#define HEX_TO_CHAR(A)  ((((A) & 0x0F)<10)? (((A) & 0x0F) + '0'): (((A) & 0x0F) - 10 + 'A'))

/*!
    @brief Get string length
    @param[in] pcStr    String
    @return -1 in case str is NULL
    @return Symbols count till string terminator
*/
int32_t lStrLen(const char* pcStr);

/*!
    @brief Compare strings case sensitive
    @param[in] pcStr1    String 1
    @param[in] pcStr2    String 2
    @return 0 in case the contents of some string is NULL
    @return -(copared count) in case the first character that does not match has a lower value in str1 than in str2
    @return (copared count) in case the first character that does not match has a greater value in str1 than in str2 or simbols are equal
 */
int32_t lStrCmp(const char* pcStr1, const char* pcStr2);

/*!
    @brief Compare strings case not sensitive
    @param[in] pcStr1    String 1
    @param[in] pcStr2    String 2
    @return 0 in case the contents of some string is NULL
    @return -(copared count) in case the first character that does not match has a lower value in str1 than in str2
    @return (copared count) in case the first character that does not match has a greater value in str1 than in str2 or simbols are equal
 */
int32_t lStrCaseCmp(const char* pcStr1, const char* pcStr2);

/*!
    @brief Compare two strings
    @param[in] pcStr1    String 1
    @param[in] pcStr2    String 2
    @return 1 in case the contents of both strings are equal and not NULL
    @return 0 in other case
 */
int32_t lStrEqual(const char* pcStr1, const char* pcStr2);

/*!
    @brief Copy string into user buffer
    @param[out] pcBuffer   User buffer
    @param[in] pcStr       String
    @return -1 in case buffer is NULL or sring is NULL or pointers are equal
    @return Symbols copied
*/
int32_t lStrCpy(char* pcBuffer, const char* pcStr);

/*!
    @brief Copy specified length of string into user buffer
    @param[out] pcBuffer   User buffer
    @param[in] lCount      Data count
    @param[in] pcStr       String
    @return -1 in case buffer is NULL or sring is NULL or lCount <= 0 or pointers are equal
    @return Symbols copied
*/
int32_t lStrnCpy(char* pcBuffer, int32_t lCount, const char* pcStr);

/*!
    @brief Concatenate two strings
    @param[out] pcBuffer Result string
    @param[in] pcStr1    String 1
    @param[in] pcStr2    String 2
    @return -1 in case wrong parameters (buf is NULL or some of strings is NULL or buffer pointer and some of string are the same)
    @return Summary length of concatenated strings
 */
int32_t lStrCat(char* pcBuffer, const char* pcStr1, const char* pcStr2);

/*!
    @brief Search first entry string 2 in string 1
    @param[in] pcStr1   String 1
    @param[in] pcStr2   String 2
    @return -1 in case of some of strings is NULL or string 2 is empty or no one entry string 2 in string 1
    @return Offset in string 1 to first entry string 2
 */
int32_t lStrSrc(const char* pcStr1, const char* pcStr2);

/*!
    @brief Search first entry string 2 in string 1 with in specified length
    @param[in] pcStr1   String 1
    @param[in] lLength  String 1 length
    @param[in] pcStr2   String 2
    @return -1 in case of some of strings is NULL or string 2 is empty or no one entry string 2 in string 1
    @return Offset in string 1 to first entry string 2
 */
int32_t lStrnSrc(const char* pcStr1, int32_t lLength, const char* pcStr2);

/*!
    @brief Search last entry string 2 in string 1 with in specified length
    @param[in] pcStr1   String 1
    @param[in] lLength  String 1 length
    @param[in] pcStr2   String 2
    @return -1 in case some of strings is NULL or string 2 is empty or no one entry string 2 in string 1
    @return Offset in string 1 to last entry string 2
 */
int32_t lStrnSrcLast(const char* pcStr1, int32_t lLength, const char* pcStr2);

/*!
    @brief Places string representation of an integer value in the buffer
    @param[in] pcBufer   User buffer
    @param[in] lValue    Integer value
    @return Characters count, -1 in case of error
 */
int32_t lLongToStr(char* pcBufer, int32_t lValue);

/*!
    @brief Parse string representation of an integer in to value
    @param[out] plOutValue Integer buffer
    @param[in] pcStrValue  Integer string
    @param[in] ucBase      Notation (2,8,10,16)
    @return Characters parced
 */
int32_t lStrToLong(uint32_t *plOutValue, char *pcStrValue, uint8_t ucBase);

/*!
    @brief Convert string to lower case
    @param[in/out] pcStr  String
 */
void vStrToLowerCase(char *pcStr);

/*!
    @brief Convert string to upper case
    @param[in/out] pcStr  String
 */
void vStrToUpperCase(char *pcStr);

/*!
  Snake notation
*/

/*!
    @brief Get string length
    @param[in] str    String
    @return -1 in case str is NULL
    @return Symbols count till string terminator
*/
int32_t str_len(const char *str);

/*!
    @brief Compare strings case sensitive
    @param[in] str1    String 1
    @param[in] str2    String 2
    @return 0 in case the contents of some string is NULL
    @return -(copared count) in case the first character that does not match has a lower value in str1 than in str2
    @return (copared count) in case the first character that does not match has a greater value in str1 than in str2 or simbols are equal
 */
int32_t str_cmp(const char *str1, const char *str2);

/*!
    @brief Compare strings case not sensitive
    @param[in] str1    String 1
    @param[in] str2    String 2
    @return 0 in case the contents of some string is NULL
    @return -(copared count) in case the first character that does not match has a lower value in str1 than in str2
    @return (copared count) in case the first character that does not match has a greater value in str1 than in str2 or simbols are equal
 */
int32_t str_case_cmp(const char *str1, const char *str2);

/*!
    @brief Compare two strings
    @param[in] str1    String 1
    @param[in] str2    String 2
    @return 1 in case the contents of both strings are equal and not NULL
    @return 0 in other case
 */
int32_t str_equal(const char *str1, const char *str2);

/*!
    @brief Copy string into user buffer
    @param[out] buffer   User buffer
    @param[in] str       String
    @return -1 in case buffer is NULL or sring is NULL or pointers are equal
    @return Symbols copied
*/
int32_t str_cpy(char *buffer, const char *str);

/*!
    @brief Copy specified length of string into user buffer
    @param[out] buffer   User buffer
    @param[in] count     Data count
    @param[in] str       String
    @return -1 in case buffer is NULL or sring is NULL or lCount <= 0 or pointers are equal
    @return Symbols copied
*/
int32_t strn_cpy(char *buffer, int32_t count, const char *str);

/*!
    @brief Concatenate two strings
    @param[out] buffer Result string
    @param[in] str1    String 1
    @param[in] str2    String 2
    @return -1 in case wrong parameters (buf is NULL or some of strings is NULL or buffer pointer and some of string are the same)
    @return Summary length of concatenated strings
 */
int32_t str_cat(char *buffer, const char *str1, const char *str2);

/*!
    @brief Search first entry string 2 in string 1
    @param[in] str1   String 1
    @param[in] str2   String 2
    @return -1 in case of some of strings is NULL or string 2 is empty or no one entry string 2 in string 1
    @return Offset in string 1 to first entry string 2
 */
int32_t str_src(const char *str1, const char *str2);

/*!
    @brief Search first entry string 2 in string 1 with in specified length
    @param[in] str1   String 1
    @param[in] length String 1 length
    @param[in] str2   String 2
    @return -1 in case of some of strings is NULL or string 2 is empty or no one entry string 2 in string 1
    @return Offset in string 1 to first entry string 2
 */
int32_t strn_src(const char *str1, int32_t length, const char *str2);

/*!
    @brief Search last entry string 2 in string 1 with in specified length
    @param[in] str1   String 1
    @param[in] length  String 1 length
    @param[in] str2   String 2
    @return -1 in case some of strings is NULL or string 2 is empty or no one entry string 2 in string 1
    @return Offset in string 1 to last entry string 2
 */
int32_t strn_src_last(const char *str1, int32_t length, const char *str2);

/*!
    @brief Places string representation of an integer value in the buffer
    @param[in] bufer   User buffer
    @param[in] value    Integer value
    @return Characters count, -1 in case of error
 */
int32_t long_to_str(char *bufer, int32_t value);

/*!
    @brief Parse string representation of an integer in to value
    @param[out] value    Integer buffer
    @param[in] str_value Integer string
    @param[in] base      
    @return Characters parced
 */
int32_t str_to_long(uint32_t *value, char *str_value, uint8_t base);

/*!
    @brief Convert string to lower case
    @param[in/out] str  String
 */
void str_to_lower_case(char *str);

/*!
    @brief Convert string to upper case
    @param[in/out] str  String
 */
void str_to_upper_case(char *str);




#ifdef __cplusplus
}
#endif

#endif /* STR_H_INCLUDED */
