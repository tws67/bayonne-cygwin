// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ucommon.
//
// GNU ucommon is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ucommon is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ucommon.  If not, see <http://www.gnu.org/licenses/>.

/**
 * A common string class and character string support functions.
 * Ucommon offers a simple string class that operates through copy-on-write 
 * when needing to expand buffer size.  Derived classes and templates allows
 * one to create strings which live entirely in the stack frame rather
 * than using the heap.  This offers the benefit of the string class
 * manipulative members without compromising performance or locking issues
 * in threaded applications.  Other things found here include better and
 * safer char array manipulation functions.
 * @file ucommon/string.h
 */

#ifndef	_UCOMMON_STRING_H_
#define	_UCOMMON_STRING_H_

#ifndef	_UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#ifndef	_UCOMMON_SOCKET_H_
#include <ucommon/socket.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef	HAVE_DIRENT_H
#include <dirent.h>
#endif

NAMESPACE_UCOMMON

/**
 * A convenience class for size of strings.
 */
typedef	unsigned short strsize_t;

class StringFormat;

/**
 * A copy-on-write string class that operates by reference count.  This string
 * class anchors a counted object that is managed as a copy-on-write
 * instance of the string data.  This means that multiple instances of the
 * string class can refer to the same string in memory if it has not been
 * modifed, which reduces heap allocation.  The string class offers functions
 * to manipulate both the string object, and generic safe string functions to
 * manipulate ordinary null terminated character arrays directly in memory. 
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT string : public Object
{
protected:
	friend class StringFormat;

	/**
	 * This is an internal class which contains the actual string data
	 * along with some control fields.  The string can be either NULL
	 * terminated or filled like a Pascal-style string, but with a user
	 * selected fill character.  The cstring object is an overdraft
	 * object, as the actual string text which is of unknown size follows 
	 * immediately after the class control data.  This class is primarely
	 * for internal use.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */

public:
	class __EXPORT cstring : public CountedObject
	{
	public:
#pragma pack(1)
		strsize_t max;	/**< Allocated size of cstring text */
		strsize_t len;  /**< Current length of cstring text */
		char fill;		/**< Filler character or 0 for none */
		char text[1];	/**< Null terminated text, in overdraft space */
#pragma pack()

		/**
		 * Create a cstring node allocated for specified string size.  The
		 * new operator would also need the size as an overdraft value.
		 * @param size of string.
		 */
		cstring(strsize_t size);

		/**
		 * Create a filled cstring node allocated for specified string size.  
		 * The new operator would also need the size as an overdraft value.
		 * The newly allocated string is filled with the fill value.
		 * @param size of string.
		 * @param fill character value to fill string with.
		 */
		cstring(strsize_t size, char fill);

		/**
		 * Used to clear a string.  If null terminated, then the string ends
		 * at the offset, otherwise it is simply filled with fill data up to
		 * the specified size.
		 * @param offset to clear from.
		 * @param size of field to clear.
		 */
		void clear(strsize_t offset, strsize_t size);

		/**
		 * Set part or all of a string with new text.
		 * @param offset to set from.
		 * @param text to insert from null terminated string.
		 * @param size of field to modify.  This is filled for fill mode.
		 */
		void set(strsize_t offset, const char *text, strsize_t size);

		/**
		 * Set our string from a string formatting object.
		 * @param format object to set from.
		 */
		void set(const StringFormat& format);

		/**
		 * Set our string from null terminated text up to our allocated size.
		 * @param text to set from.
		 */
		void set(const char *text);

		/**
		 * Append null terminated text to our string buffer.
		 * @param text to append.
		 */
		void add(const char *text);

		/**
		 * Append text to string buffer from formatting object.
		 * @param format object to add from.
		 */
		void add(const StringFormat &format);

		/**
		 * Append a single character to our string buffer.
		 * @param character to append.
		 */
		void add(char character);

		/**
		 * Fill our string buffer to end if fill mode.
		 */
		void fix(void);

		/**
		 * Trim filler at end to reduce filled string to null terminated
		 * string for further processing.
		 */
		void unfix(void);

		/**
		 * Adjust size of our string buffer by deleting characters from
		 * start of buffer.
		 * @param number of characters to delete.
		 */ 
		void inc(strsize_t number);

		/**
		 * Adjust size of our string buffer by deleting characters from
		 * end of buffer.
		 * @param number of characters to delete.
		 */ 
		void dec(strsize_t number);
	};

protected:
	cstring *str;  /**< cstring instance our object references. */

	/**
	 * Factory create a cstring object of specified size.
	 * @param size of allocated space for string buffer.
	 * @param fill character to use or 0 if null.
	 * @return new cstring object.
	 */
	cstring *create(strsize_t size, char fill = 0) const;

	/**
	 * Compare the values of two string.  This is a virtual so that it
	 * can be overriden for example if we want to create strings which
	 * ignore case, or which have special ordering rules.
	 * @param string to compare with.
	 * @return 0 if equal, <0 if less than, 0> if greater than.
	 */
	virtual int compare(const char *string) const;

	/**
	* Test if two string values are equal.
	* @param string to compare with.
	* @return true if equal.
	*/
	bool equal(const char *string) const;

	/**
	 * Increase retention of our reference counted cstring.  May be overriden
	 * for memstring which has fixed cstring object.
	 */
	virtual void retain(void);

	/**
	 * Decrease retention of our reference counted cstring.  May be overriden
	 * for memstring which has fixed cstring object.
	 */
	virtual void release(void);

	/**
	 * Return cstring to use in copy constructors.  Is virtual for memstring.
	 * @return cstring for copy constructor.
	 */
	virtual cstring *c_copy(void) const;

	/**
	 * Copy on write operation for cstring.  This always creates a new
	 * unique copy for write/modify operations and is a virtual for memstring
	 * to disable.
	 * @param size to add to allocated space when creating new cstring.
	 */
	virtual void cow(strsize_t size = 0);

	strsize_t getStringSize(void);

public:
	/**
	 * A constant for an invalid position value.
	 */
	static const strsize_t npos;

	/**
	 * Create a new empty string object.
	 */
	string();

	/**
	 * Create a string from a formatting object.
	 * @param format object to use in creating string.
	 */
	string(const StringFormat& format);

	/**
	 * Create an empty string with a buffer pre-allocated to a specified size.
	 * @param size of buffer to allocate.
	 */
	string(strsize_t size);

	/**
	 * Create a filled string with a buffer pre-allocated to a specified size.
	 * @param size of buffer to allocate.
	 * @param fill character to use.
	 */
	string(strsize_t size, char fill);

	/**
	 * Create a string by printf-like formating into a pre-allocated space
	 * of a specified size.
	 * @param size of buffer to allocate.
	 * @param format control for string.
	 */
	string(strsize_t size, const char *format, ...) __PRINTF(3, 4);

	/**
	 * Create a string from null terminated text.
	 * @param text to use for string.
	 */ 
	string(const char *text);

	/**
	 * Create a string from null terminated text up to a maximum specified
	 * size.
	 * @param text to use for string.
	 * @param size limit of new string.
	 */ 
	string(const char *text, strsize_t size);

	/**
	 * Create a string for a substring.  The end of the substring is a
	 * pointer within the substring itself.
	 * @param text to use for string.
	 * @param end of text in substring.
	 */
	string(const char *text, const char *end);

	/**
	 * Construct a copy of a string object.  Our copy inherets the same 
	 * reference counted instance of cstring as in the original.
	 * @param existing string to copy from.
	 */
	string(const string& existing);

	/**
	 * Destroy string.  De-reference cstring.  If last reference to cstring, 
	 * then also remove cstring from heap.
	 */
	virtual ~string();

	/**
	 * Get a new string object as a substring of the current object.
	 * @param offset of substring.
	 * @param size of substring or 0 if to end.
	 * @return string object holding substring.
	 */
	string get(strsize_t offset, strsize_t size = 0) const;

	/**
	 * Scan input items from a string object.
	 * @param format string of input to scan.
	 * @return number of items scanned.
	 */
	int scanf(const char *format, ...) __SCANF(2, 3);

	/**
	 * Scan input items from a string object.
	 * @param format string of input to scan.
	 * @param args list to scan into.
	 * @return number of items scanned.
	 */
	int vscanf(const char *format, va_list args) __SCANF(2, 0);

	/**
	 * Print items into a string object.
	 * @param format string of print format.
	 * @return number of bytes written to string.
	 */
	strsize_t printf(const char *format, ...) __PRINTF(2, 3);

	/**
	 * Print items into a string object.
	 * @param format string of print format.
	 * @param args list to print.
	 * @return number of bytes written to string.
	 */
	strsize_t vprintf(const char *format, va_list args) __PRINTF(2, 0);

	/**
	 * Get memory text buffer of string object.
	 * @return writable string buffer.
	 */
	char *c_mem(void) const;

	/**
	 * Get character text buffer of string object.
	 * @return character text buffer.
	 */
	const char *c_str(void) const;

	/**
	 * Resize and re-allocate string memory.
	 * @param size to allocate for string.
	 * @return true if re-allocated.  False in derived memstring.
	 */
	virtual bool resize(strsize_t size);

	/**
	 * Set string object to text of a null terminated string.
	 * @param text string to set.
	 */
	void set(const char *text);

	/**
	 * Set string object to text of a formatted object.
	 * @param format object to use.
	 */
	void set(const StringFormat& format);

	/**
	 * Set a portion of the string object at a specified offset to a text
	 * string.
	 * @param offset in object string buffer.
	 * @param text to set at offset.
	 * @param size of text area to set or 0 until end of text.
	 */
	void set(strsize_t offset, const char *text, strsize_t size = 0);

	/**
	 * Set a text field within our string object.
	 * @param text to set.
	 * @param overflow character to use as filler if text is too short.
	 * @param offset in object string buffer to set text at.
	 * @param size of part of buffer to set with text and overflow.
	 */
	void set(const char *text, char overflow, strsize_t offset, strsize_t size = 0);

	/**
	 * Set a text field within our string object offset from the end of buffer.
	 * @param text to set.
	 * @param overflow character to use as filler if text is too short.
	 * @param offset from end of object string buffer to set text at.
	 * @param size of part of buffer to set with text and overflow.
	 */
	void rset(const char *text, char overflow, strsize_t offset, strsize_t size = 0);

	/**
	 * Append formatted object to our string buffer.
	 * @param format object to append.
	 */
	void add(const StringFormat& format);


	/**
	 * Append null terminated text to our string buffer.
	 * @param text to append.
	 */
	void add(const char *text);

	/**
	 * Append a single character to our string buffer.
	 * @param character to append.
	 */
	void add(char character);

	/**
	 * Trim lead characters from the string.
	 * @param list of characters to remove.
	 */
	void trim(const char *list);

	/**
	 * Chop trailing characters from the string.
	 * @param list of characters to remove.
	 */
	void chop(const char *list);

	/**
	 * Strip lead and trailing characters from the string.
	 * @param list of characters to remove.
	 */
	void strip(const char *list);

	/**
	 * Unquote a quoted string.  Removes lead and trailing quote marks.
	 * @param quote pairs of characters for open and close quote.
	 * @return true if string was quoted.
	 */
	bool unquote(const char *quote);

	/**
	 * Cut (remove) text from string.
	 * @param offset to start of text field to remove.
	 * @param size of text field to remove or 0 to remove to end of string.
	 */
	void cut(strsize_t offset, strsize_t size = 0);

	/**
	 * Clear a field of a filled string with filler.
	 * @param offset to start of field to clear.
	 * @param size of field to fill or 0 to fill to end of string.
	 */
	void clear(strsize_t offset, strsize_t size = 0);

	/**
	 * Clear string by setting to empty.
	 */
	void clear(void);

	/**
	 * Convert string to upper case.
	 */
	void upper(void);

	/**
	 * Convert string to lower case.
	 */
	void lower(void);

	/**
	 * Count number of occurrances of characters in string.
	 * @param list of characters to find.
	 * @return count of instances of characters in string.
	 */
	strsize_t ccount(const char *list) const;

	/**
	 * Count all characters in the string (strlen).
	 * @return count of characters.
	 */
	strsize_t count(void) const;

	/**
	 * Get the size of currently allocated space for string.
	 * @return size allocated for text.
	 */
	strsize_t size(void) const;

	/**
	 * Find offset of a pointer into our string buffer.  This can be used
	 * to find the offset position of a pointer returned by find, for
	 * example.  This is used when one needs to convert a member function
	 * that returns a pointer to call a member function that operates by
	 * a offset value.  If the pointer is outside the range of the string
	 * then npos is returned.
	 * @param pointer into our object's string buffer.
	 */
	strsize_t offset(const char *pointer) const;

	/**
	 * Return character found at a specific position in the string.
	 * @param position in string, negative values computed from end.
	 * @return character code at specified position in string.
	 */
	char at(int position) const;

	/**
	 * Find last occurance of a character in the string.
	 * @param list of characters to search for.
	 * @return pointer to last occurance from list or NULL.
	 */
	const char *last(const char *list) const;

	/**
	 * Find first occurance of a character in the string.
	 * @param list of characters to search for.
	 * @return pointer to first occurance from list or NULL.
	 */
	const char *first(const char *list) const;

	/**
	 * Get pointer to first character in string for iteration.
	 * @return first character pointer or NULL if empty.
	 */
	const char *begin(void) const;

	/**
	 * Get pointer to last character in string for iteration.
	 * @return last character pointer or NULL if empty.
	 */
	const char *end(void) const;

	/**
	 * Skip lead characters in the string.
	 * @param list of characters to skip when found.
	 * @param offset to start of scan.
	 * @return pointer to first part of string past skipped characters.
	 */
	const char *skip(const char *list, strsize_t offset = 0) const;

	/**
	 * Skip trailing characters in the string.  This searches the
	 * string in reverse order.
	 * @param list of characters to skip when found.
	 * @param offset to start of scan.  Default is end of string.
	 * @return pointer to first part of string before skipped characters.
	 */
	const char *rskip(const char *list, strsize_t offset = npos) const;

	/**
	 * Find a character in the string.
	 * @param list of characters to search for.
	 * @param offset to start of search.
	 * @return pointer to first occurance of character.
	 */
	const char *find(const char *list, strsize_t offset = 0) const;

	/**
	 * Find last occurance of character in the string.
	 * @param list of characters to search for.
	 * @param offset to start of search.  Default is end of string.
	 * @return pointer to last occurance of character.
	 */
	const char *rfind(const char *list, strsize_t offset = npos) const;

	/**
	 * Split the string by a pointer position.  Everything after the pointer
	 * is removed.
	 * @param pointer to split position in string.
	 */
	void split(const char *pointer);

	/**
	 * Split the string at a specific offset.  Everything after the offset
	 * is removed.
	 * @param offset to split position in string.
	 */
	void split(strsize_t offset);

	/**
	 * Split the string by a pointer position.  Everything before the pointer
	 * is removed.
	 * @param pointer to split position in string.
	 */
	void rsplit(const char *pointer);

	/**
	 * Split the string at a specific offset.  Everything before the offset
	 * is removed.
	 * @param offset to split position in string.
	 */
	void rsplit(strsize_t offset);

	/**
	 * Find pointer in string where specified character appears.
	 * @param character to find.
	 * @return string pointer for character if found, NULL if not.
	 */
	const char *chr(char character) const;

	/**
	 * Find pointer in string where specified character last appears.
	 * @param character to find.
	 * @return string pointer for last occurance of character if found, 
	 * NULL if not.
	 */
	const char *rchr(char character) const;

	/**
	 * Get length of string.
	 * @return length of string.
	 */
	strsize_t len(void);

	/**
	 * Get filler character used for field array strings.
	 * @return filler character or 0 if none.
	 */
	char fill(void);

	/**
	 * Casting reference to raw text string.
	 * @return null terminated text of string.
	 */
	inline operator const char *() const
		{return c_str();};

	/**
	 * Reference raw text buffer by pointer operator.
	 * @return null terminated text of string.
	 */
	inline const char *operator*() const
		{return c_str();};

	/**
	 * Test if the string's allocated space is all used up.
	 * @return true if no more room for append.
	 */
	bool full(void) const;

	/**
	 * Get a new substring through object expression.
	 * @param offset of substring.
	 * @param size of substring or 0 if to end.
	 * @return string object holding substring.
	 */
	string operator()(int offset, strsize_t size) const;

	/**
	 * Reference a string in the object by relative offset.  Positive
	 * offsets are from the start of the string, negative from the
	 * end.
	 * @param offset to string position.
	 * @return pointer to string data or NULL if invalid offset.
	 */
	const char *operator()(int offset) const;

	/**
	 * Reference a single character in string object by array offset.
	 * @param offset to character.
	 * @return character value at offset.
	 */
	const char operator[](int offset) const;

	/**
	 * Test if string is empty.
	 * @return true if string is empty.
	 */
	bool operator!() const;

	/**
	 * Test if string has data.
	 * @return true if string has data.
	 */
	operator bool() const;

	/**
	 * Create new cow instance and assign value from another string object.
	 * @param object to assign from.
	 * @return our object for expression use.
	 */
	string& operator^=(const string& object);

	/**
	 * Create new cow instance and assign value from formatted string object.
	 * @param format object to assign from.
	 * @return our object for expression use.
	 */
	string& operator^=(const StringFormat& format);

	/**
	 * Create new cow instance and assign value from null terminated text.
	 * @param text to assign from.
	 * @return our object for expression use.
	 */
	string& operator^=(const char *text);

	/**
	 * Concatenate null terminated text to our object.  This creates a new
	 * copy-on-write instance to hold the concatenated string.
	 * @param text to concatenate.
	 */
	string& operator+(const char *text);

	/**
	 * Concatenate string formatted object to our object.  This creates a new
	 * copy-on-write instance to hold the concatenated string.
	 * @param format object to concatenate.
	 */
	string& operator+(const StringFormat& format);

	/**
	 * Concatenate null terminated text to our object.  This directly
	 * appends the text to the string buffer and does not resize the
	 * object if the existing cstring allocation space is fully used.
	 * @param text to concatenate.
	 */
	string& operator&(const char *text);

	/**
	 * Concatenate formatted string object to our object.  This directly
	 * appends the text to the string buffer and does not resize the
	 * object if the existing cstring allocation space is fully used.
	 * @param format object to concatenate.
	 */
	string& operator&(const StringFormat& format);

	/**
	 * Assign our string with the cstring of another object.  If we had
	 * an active string reference, it is released.  The object now has
	 * a duplicate reference to the cstring of the other object.
	 * @param object to assign from.
	 */
	string& operator=(const string& object);

	/**
	 * Assign our string from a string formatting object.
	 * @param format object to assign from.
	 */
	string& operator=(const StringFormat& format);

	/**
	 * Assign text to our existing buffer.  This performs a set method.
	 * @param text to assign from.
	 */
	string& operator=(const char *text);

	/**
	 * Delete first character from string.
	 */
	string& operator++(void);

	/**
	 * Delete a specified number of characters from start of string.
	 * @param number of characters to delete.
	 */
	string& operator+=(strsize_t number);

	/**
	 * Delete last character from string.
	 */
	string& operator--(void);

	/**
	 * Delete a specified number of characters from end of string.
	 * @param number of characters to delete.
	 */
	string& operator-=(strsize_t number);

	/**
	 * Compare our object with null terminated text.
	 * @param text to compare with.
	 * @return true if we are equal.
	 */
	bool operator==(const char *text) const;

	/**
	 * Compare our object with null terminated text.  Compare method is used.
	 * @param text to compare with.
	 * @return true if we are not equal.
	 */
	bool operator!=(const char *text) const;

	/**
	 * Compare our object with null terminated text.  Compare method is used.
	 * @param text to compare with.
	 * @return true if we are less than text.
	 */
	bool operator<(const char *text) const;

	/**
	 * Compare our object with null terminated text.  Compare method is used.
	 * @param text to compare with.
	 * @return true if we are less than or equal to text.
	 */
	bool operator<=(const char *text) const;

	/**
	 * Compare our object with null terminated text.  Compare method is used.
	 * @param text to compare with.
	 * @return true if we are greater than text.
	 */
	bool operator>(const char *text) const;

	/**
	 * Compare our object with null terminated text.  Compare method is used.
	 * @param text to compare with.
	 * @return true if we are greater than or equal to text.
	 */
	bool operator>=(const char *text) const;

	/**
	 * Scan input items from a string object.
	 * @param object to scan from.
	 * @param format string of input to scan.
	 * @return number of items scanned.
	 */
	static int scanf(string& object, const char *format, ...) __SCANF(2, 3);

	/**
	 * Print formatted items into a string object.
	 * @param object to print into.
	 * @param format string to print with.
	 * @return number of bytes written into object.
	 */
	static strsize_t printf(string& object, const char *format, ...) __PRINTF(2, 3);

	/**
	 * Read arbitrary binary data from socket into a string object.  The 
	 * total number of bytes that may be read is based on the allocated
	 * size of the object.
	 * @param socket to read from.
	 * @param object to save read data.
	 * @return number of bytes read.
	 */
	static int read(Socket& socket, string& object);
	
	/**
	 * Write the string object to a socket.
	 * @param socket to write to.
	 * @param object to get data from.
	 * @return number of bytes written.
	 */
	static int write(Socket& socket, string& object);

	/**
	 * Read arbitrary binary data from a file into a string object.  The 
	 * total number of bytes that may be read is based on the allocated
	 * size of the object.
	 * @param file to read from.
	 * @param object to save read data.
	 * @return number of bytes read.
	 */
	static int read(FILE *file, string& object);

	/**
	 * Write the string object to a file.
	 * @param file to write to.
	 * @param object to get data from.
	 * @return number of bytes written.
	 */
	static int write(FILE *file, string& object); 

	/**
	 * Read a line of text input from a socket into the object.  The 
	 * maximum number of bytes that may be read is based on the currently
	 * allocated size of the object.
	 * @param socket to read from.
	 * @param object to save read data.
	 * @return false if end of file.
	 */
	static bool getline(Socket& socket, string& object);

	/**
	 * Write string as a line of text data to a socket.  A newline will be 
	 * appended to the end.
	 * @param socket to print to.
	 * @param object to get text line from.
	 * @return true if successful.
	 */
	static bool putline(Socket& socket, string& object);

	/**
	 * Read a line of text input from a file into the object.  The 
	 * maximum number of bytes that may be read is based on the currently
	 * allocated size of the object.
	 * @param file to read from.
	 * @param object to save read data.
	 * @return false if end of file.
	 */
	static bool getline(FILE *file, string& object);

	/**
	 * Write string as a line of text data to a file.  A newline will be 
	 * appended to the end.
	 * @param file to print to.
	 * @param object to get text line to put into file.
	 * @return true if successful.
	 */
	static bool putline(FILE *file, string& object);

	/**
	 * Swap the cstring references between two strings.
	 * @param object1 to swap.
	 * @param object2 to swap.
	 */
	static void swap(string& object1, string& object2);
	
	/**
	 * Fix and reset string object filler.
	 * @param object to fix.
	 */
	static void fix(string& object);

	/**
	 * Convert null terminated text to lower case.
	 * @param text to convert.
	 */
	static void lower(char *text);
	
	/**
	 * Convert null terminated text to upper case.
	 * @param text to convert.
	 */
	static void upper(char *text);

	/**
	 * A thread-safe token parsing routine for null terminated strings.  This 
	 * is related to strtok, but with safety checks for NULL values and a
	 * number of enhancements including support for quoted text that may
	 * contain token seperators within quotes.  The text string is modified 
	 * as it is parsed.
	 * @param text string to examine for tokens.
	 * @param last token position or set to NULL for start of string.
	 * @param list of characters to use as token seperators.
	 * @param quote pairs of characters for quoted text or NULL if not used.
	 * @param end of line marker characters or NULL if not used.
	 * @return token extracted from string or NULL if no more tokens found.
	 */
	static char *token(char *text, char **last, const char *list, const char *quote = NULL, const char *end = NULL);

	/**
	 * Skip after lead characters in a null terminated string.
	 * @param text pointer to start at.
	 * @param list of characters to skip when found.
	 * @return pointer to first part of string past skipped characters.
	 */
	static char *skip(char *text, const char *list);

	/**
	 * Skip before trailing characters in a null terminated string.
	 * @param text pointer to start at.
	 * @param list of characters to skip when found.
	 * @return pointer to last part of string past skipped characters.
	 */
	static char *rskip(char *text, const char *list);

	/**
	 * Unquote a quoted null terminated string.  Returns updated string
	 * position and replaces trailing quote with null byte if quoted.
	 * @param text to examine.
	 * @param quote pairs of character for open and close quote.
	 * @return new text pointer if quoted, NULL if unchanged.
	 */
	static char *unquote(char *text, const char *quote);

	/**
	 * Set a field in a null terminated string relative to the end of text.
	 * @param buffer to modify.
	 * @param size of field to set.
	 * @param text to replace end of string with.
	 * @return pointer to text buffer.
	 */
	static char *rset(char *buffer, size_t size, const char *text);
	
	/**
	 * Safely set a null terminated string buffer in memory.  If the text
	 * is too large to fit into the buffer, it is truncated to the size.
	 * @param buffer to set.
	 * @param size of buffer.  Includes null byte at end of string.
	 * @param text to set in buffer.
	 * @return pointer to text buffer.
	 */
	static char *set(char *buffer, size_t size, const char *text);
	
    /**
     * Safely set a null terminated string buffer in memory.  If the text
     * is too large to fit into the buffer, it is truncated to the size.
     * @param buffer to set.
     * @param size of buffer.  Includes null byte at end of string.
     * @param text to set in buffer.
	 * @param max size of text to set.
     * @return pointer to text buffer.
     */
	static char *set(char *buffer, size_t size, const char *text, size_t max);

	/**
     * Safely append a null terminated string into an existing string in 
	 * memory.  If the resulting string is too large to fit into the buffer, 
	 * it is truncated to the size.
     * @param buffer to set.
     * @param size of buffer.  Includes null byte at end of string.
     * @param text to set in buffer.
     * @return pointer to text buffer.
     */
	static char *add(char *buffer, size_t size, const char *text); 

	/**
     * Safely append a null terminated string into an existing string in 
	 * memory.  If the resulting string is too large to fit into the buffer, 
	 * it is truncated to the size.
     * @param buffer to set.
     * @param size of buffer.  Includes null byte at end of string.
     * @param text to set in buffer.
	 * @param max size of text to append.
     * @return pointer to text buffer.
     */
	static char *add(char *buffer, size_t size, const char *text, size_t max);
	
	/**
	 * Find position of case insensitive substring within a string.
	 * @param text to search in.
	 * @param key string to locate.
	 * @param optional separator chars if formatted as list of keys.
	 * @return substring position if found, or NULL.
	 */
	static const char *ifind(const char *text, const char *key, const char *optional);

	/**
	 * Find position of substring within a string.
	 * @param text to search in.
	 * @param key string to locate.
	 * @param optional separator chars if formatted as list of keys.
	 * @return substring position if found, or NULL.
	 */
	static const char *find(const char *text, const char *key, const char *optional);

	/**
	 * Safe version of strlen function.  Accepts NULL as 0 length strings.
	 * @param text string.
	 * @return length of string.
	 */
	static size_t count(const char *text);

	/**
	 * Safe string comparison function.
	 * @param text1 to compare.
	 * @param text2 to compare.
	 * @return 0 if equal, >0 if text1 > text2, <0 if text1 < text2.
	 */
	static int compare(const char *text1, const char *text2);

	/**
	 * Simple equal test for strings.
	 * @param text1 to test.
	 * @param text2 to test.
	 * @return true if equal and case is same.
	 */
	static bool equal(const char *text1, const char *text2);

	/**
	 * Safe string comparison function.
	 * @param text1 to compare.
	 * @param text2 to compare.
	 * @param size limit of strings to compare.
	 * @return 0 if equal, >0 if text1 > text2, <0 if text1 < text2.
	 */
	static int compare(const char *text1, const char *text2, size_t size);

	/**
	 * Simple equal test for strings.
	 * @param text1 to test.
	 * @param text2 to test.
	 * @param size limit of strings to compare.
	 * @return true if equal and case is same.
	 */
	static bool equal(const char *text1, const char *text2, size_t size);

	/**
	 * Safe case insensitive string comparison function.
	 * @param text1 to compare.
	 * @param text2 to compare.
	 * @return 0 if equal, >0 if text1 > text2, <0 if text1 < text2.
	 */
	static int case_compare(const char *text1, const char *text2);

	/**
	 * Simple case insensitive equal test for strings.
	 * @param text1 to test.
	 * @param text2 to test.
	 * @return true if equal.
	 */
	static bool case_equal(const char *text1, const char *text2);

	/**
	 * Safe case insensitive string comparison function.
	 * @param text1 to compare.
	 * @param text2 to compare.
	 * @param size limit of strings to compare.
	 * @return 0 if equal, >0 if text1 > text2, <0 if text1 < text2.
	 */
	static int case_compare(const char *text1, const char *text2, size_t size);

	/**
	 * Simple case insenstive equal test for strings.
	 * @param text1 to test.
	 * @param text2 to test.
	 * @param size limit of strings to compare.
	 * @return true if equal.
	 */
	static bool case_equal(const char *text1, const char *text2, size_t size);

	/**
	 * Return start of string after characters to trim from beginning.
	 * This function does not modify memory.
	 * @param text buffer to examine.
	 * @param list of characters to skip from start.
	 * @return position in text past lead trim.
	 */
	static char *trim(char *text, const char *list); 

	/**
	 * Strip trailing characters from the text string.  This function will
	 * modify memory.
	 * @param text buffer to examine.
	 * @param list of characters to chop from trailing end of string.
	 * @return pointer to text buffer.
	 */
	static char *chop(char *text, const char *list); 

	/**
	 * Skip lead and remove trailing characters from a text string.  This
	 * function will modify memory.
	 * @param text buffer to examine.
	 * @param list of characters to trim and chop.
	 * @return position in text past lead trim.
	 */
	static char *strip(char *text, const char *list);

	/**
	 * Fill a section of memory with a fixed text character.  Adds a null
	 * byte at the end.
	 * @param text buffer to fill.
	 * @param size of text buffer with null terminated byte.
	 * @param character to fill with.
	 * @return pointer to text buffer.
	 */
	static char *fill(char *text, size_t size, char character);

	/**
	 * Count instances of characters in a list in a text buffer.
	 * @param text buffer to examine.
	 * @param list of characters to count in buffer.
	 * @return number of instances of the characters in buffer.
	 */
	static unsigned ccount(const char *text, const char *list);

	/**
	 * Find the first occurance of a character in a text buffer.
	 * @param text buffer to examine.
	 * @param list of characters to search for.
	 * @return pointer to first instance found or NULL.
	 */
	static char *find(char *text, const char *list);

	/**
	 * Find the last occurance of a character in a text buffer.
	 * @param text buffer to examine.
	 * @param list of characters to search for.
	 * @return pointer to last instance found or NULL.
	 */
	static char *rfind(char *text, const char *list);

	/**
	 * Get pointer to first character past character requested.
	 * @param text buffer to examine.
	 * @param list of characters.
	 * @return first character pointer past list.
	 */
	static char *first(char *text, const char *list);

	/**
	 * Get pointer to last character before character requested.
	 * @param text buffer to examine.
	 * @param list of characters.
	 * @return last character pointer past list.
	 */
	static char *last(char *text, const char *list);

	/**
	 * Duplicate null terminated text into the heap.
	 * @param text to duplicate.
	 * @return duplicate copy of text allocated from heap.
	 */
	static char *dup(const char *text);

	/**
	 * A thread-safe token parsing routine for strings objects.  This 
	 * is related to strtok, but with safety checks for NULL values and a
	 * number of enhancements including support for quoted text that may
	 * contain token seperators within quotes.  The object is modified 
	 * as it is parsed.
	 * @param object to examine for tokens.
	 * @param last token position or set to NULL for start of string.
	 * @param list of characters to use as token seperators.
	 * @param quote pairs of characters for quoted text or NULL if not used.
	 * @param end of line marker characters or NULL if not used.
	 * @return token extracted from string or NULL if no more tokens found.
	 */
	inline static char *token(string& object, char **last, const char *list, const char *quote = NULL, const char *end = NULL)
		{return token(object.c_mem(), last, list, quote, end);};

	/**
	 * Scan input items from a string object.
	 * @param object to scan.
	 * @param format string of input to scan.
	 * @param args list to scan into.
	 * @return number of items scanned.
	 */
	__SCANF(2,0) inline static int vscanf(string& object, const char *format, va_list args)
		{return object.vscanf(format, args);}

	/**
	 * Print items into a string object.
	 * @param object to print into.
	 * @param format string of print format.
	 * @param args list to print.
	 * @return number of bytes written to string.
	 */
	__PRINTF(2,0) inline static strsize_t vprintf(string& object, const char *format, va_list args)
		{return object.vprintf(format, args);}

	/**
	 * Count all characters in the string object (strlen).
	 * @param object to count.
	 * @return count of characters.
	 */
	inline static strsize_t len(string& object)
		{return object.len();};

	/**
	 * Get memory text buffer of string object.
	 * @param object to get string buffer from.
	 * @return writable string buffer.
	 */
	inline static char *mem(string& object)
		{return object.c_mem();};

	/**
	 * Get the size of currently allocated space for string.
	 * @param object to examine.
	 * @return size allocated for text.
	 */
	inline static strsize_t size(string& object)
		{return object.size();};

	/**
	 * Clear a string object.
	 * @param object to clear.
	 */
	inline static void clear(string& object)
		{object.clear();};

	/**
	 * Count number of occurrances of characters in string object.
	 * @param object to examine.
	 * @param list of characters to find.
	 * @return count of instances of characters.
	 */
	inline static unsigned ccount(string& object, const char *list)
		{return object.ccount(list);};

	/**
	 * Count all characters in the string object (strlen).
	 * @param object to count.
	 * @return count of characters.
	 */
	inline static size_t count(string& object)
		{return object.count();};

	/**
	 * Convert string object to upper case.
	 * @param object to modify.
	 */
	inline static void upper(string& object)
		{object.upper();};

	/**
	 * Convert string object to lower case.
	 * @param object to modify.
	 */
	inline static void lower(string& object)
		{object.lower();};

	/**
	 * Unquote a quoted string.  Removes lead and trailing quote marks.
	 * @param object to unquote.
	 * @param quote pairs of characters for open and close quote.
	 * @return true if string was quoted.
	 */
	inline static bool unquote(string& object, const char *quote)
		{return object.unquote(quote);};

	/**
	 * Trim lead characters from the string.
	 * @param object to trim.
	 * @param list of characters to remove.
	 */
	inline static void trim(string& object, const char *list)
		{object.trim(list);};

	/**
	 * Chop trailing characters from the string.
	 * @param object to chop.
	 * @param list of characters to remove.
	 */
	inline static void chop(string& object, const char *list)
		{object.trim(list);};

	/**
	 * Strip lead and trailing characters from the string.
	 * @param object to strip.
	 * @param list of characters to remove.
	 */
	inline static void strip(string& object, const char *list)
		{object.trim(list);};

	/**
	 * Find a character in the string.
	 * @param object to search.
	 * @param list of characters to search for.
	 * @return pointer to first occurance of character.
	 */
	inline static const char *find(string& object, const char *list)
		{return object.find(list);};

	/**
	 * Find last character in the string.
	 * @param object to search.
	 * @param list of characters to search for.
	 * @return pointer to last occurance of character.
	 */
	inline static const char *rfind(string& object, const char *list)
		{return object.rfind(list);};

	/**
	 * Get pointer to first character past character requested.
	 * @param object to examine.
	 * @param list of characters.
	 * @return first character pointer past list.
	 */
	inline static const char *first(string& object, const char *list)
		{return object.first(list);};

	/**
	 * Get pointer to last character past character requested.
	 * @param object to examine.
	 * @param list of characters.
	 * @return last character pointer before list.
	 */
	inline static const char *last(string& object, const char *list)
		{return object.last(list);};

	/**
	 * Convert string to a double value.
	 * @param object to convert.
	 * @param pointer to update with end of parsed value.
	 * @return double value of object.
	 */
	inline static double tod(string& object, char **pointer = NULL)
		{return strtod(mem(object), pointer);};

	/**
	 * Convert string to a long value.
	 * @param object to convert.
	 * @param pointer to update with end of parsed value.
	 * @return long value of object.
	 */
	inline static long tol(string& object, char **pointer = NULL)
		{return strtol(mem(object), pointer, 0);};

	/**
	 * Convert text to a double value.
	 * @param text to convert.
	 * @param pointer to update with end of parsed value.
	 * @return double value of object.
	 */
	inline static double tod(const char *text, char **pointer = NULL)
		{return strtod(text, pointer);};

	/**
	 * Convert text to a long value.
	 * @param text to convert.
	 * @param pointer to update with end of parsed value.
	 * @return long value of object.
	 */
	inline static long tol(const char *text, char **pointer = NULL)
		{return strtol(text, pointer, 0);};
};

/**
 * A string class that uses a cstring buffer that is fixed in memory.
 * This allows one to manipulate a fixed buffer of text in memory through
 * the string class.  The size of the memory used must include space for
 * the overhead() size needed for the cstring object control data.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT memstring : public string
{
public:
	static const size_t header;

private:
	bool resize(strsize_t size);
	void cow(strsize_t adj = 0);
	void release(void);

protected:
	cstring *c_copy(void) const;

public:
	/**
	 * Assign the text of a string to our object.
	 * @param object to copy text from.
	 */
	inline void operator=(string& object)
		{set(object.c_str());};

	/**
	 * Assign null terminated text to our object.
	 * @param text to copy.
	 */
	inline void operator=(const char *text)
		{set(text);};

	/**
	 * Create an instance of a memory string.
	 * @param memory to use for cstring object.
	 * @param size of string.  Total size must include space for overhead.
	 * @param fill character for fixed character fields.
	 */
	memstring(void *memory, strsize_t size, char fill = 0);

	/**
	 * Destroy memory string.
	 */
	~memstring();

	/**
	 * Create a memory string with memory allocated from the heap.
	 * @param size of string to allocate.  Automatically adds control size.
	 * @param fill character for fixed field strings.
	 */
	static memstring *create(strsize_t size, char fill = 0);

	/**
	 * Create a memory string with memory allocated from a pager.
	 * @param pager to allocate memory from.
	 * @param size of string to allocate.  Automatically adds control size.
	 * @param fill character for fixed field strings.
	 */
	static memstring *create(mempager *pager, strsize_t size, char fill = 0);
};

/**
 * A string conversion class for use as a base class in objects which can
 * transform themselves into string representations.  This can be used
 * together with the string class for automatic conversions.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT StringFormat
{
protected:
	friend class string;
	friend class string::cstring;

	virtual ~StringFormat();

	/**
	 * Convert derived object into a string buffer.  This may be a public
	 * method in a derived class.
	 * @param buffer to save representation in.
	 * @param size of buffer to use.
	 */
	virtual void put(char *buffer, size_t size) const = 0;

	/**
	 * Get maximum space that might be needed for string representation.
	 * @return space needed.
	 */
	virtual strsize_t getStringSize(void) const = 0;
};


/**
 * A template to create a character array that can be manipulated as a string.
 * This is a mini string/stringbuf class that supports a subset of
 * functionality but does not require a complex supporting object.  Like
 * stringbuf, this can be used to create local string variables.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
template<size_t S>
class charbuf
{
private:
	char buffer[S];

public:
	/**
	 * Create a new character buffer with an empty string.
	 */
	inline charbuf() 
		{buffer[0] = 0;};

	/**
	 * Create a character buffer with assigned text.  If the text is
	 * larger than the size of the object, it is truncated.
	 * @param text to assign.
	 */
	inline charbuf(const char *text) 
		{string::set(buffer, S, text);};

	/**
	 * Assign null terminated text to the object.
	 * @param text to assign.
	 */
	inline void operator=(const char *text)
		{string::set(buffer, S, text);};

	/**
	 * Concatenate text into the object.  If the text is larger than the
	 * size of the object, then it is truncated.
	 * @param text to append.
	 */
	inline void operator+=(const char *text)
		{string::add(buffer, S, text);};

	/**
	 * Test if data is contained in the object.
	 * @return true if there is text.
	 */
	inline operator bool() const
		{return buffer[0];};

	/**
	 * Test if the object is empty.
	 * @return true if the object is empty.
	 */
	inline bool operator!() const
		{return buffer[0] == 0;};	

	/**
	 * Get text by casting reference.
	 * @return pointer to text in object.
	 */
	inline operator char *()
		{return buffer;};

	/**
	 * Get text by object pointer reference.
	 * @return pointer to text in object.
	 */
	inline char *operator*()
		{return buffer;};

	/**
	 * Array operator to get a character from the object.
	 * @param offset of character in string buffer.
	 * @return character at offset.
	 */
	inline char operator[](size_t offset) const
		{return buffer[offset];};

	/**
	 * Get a pointer to an offset in the object by expression operator.
	 * @param offset of character in string buffer.
	 * @return pointer to offset in object.
	 */
	inline char *operator()(size_t offset)
		{return buffer + offset;};

	/**
	 * Get allocated size of the object.
	 * @return allocated size.
	 */
	inline size_t size(void) const
		{return S;};
};

/**
 * A string class that has a predefined string buffer.  The string class
 * and buffer are allocated together as one object.  This allows one to use
 * string objects entirely resident on the local stack as well as on the
 * heap.  Using a string class on the local stack may be more convenient
 * than a char array since one can use all the features of the class
 * including assignment and concatenation which a char buffer cannot as
 * easily do.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
template<strsize_t S>
class stringbuf : public memstring
{
private:
	char buffer[sizeof(cstring) + S];
	
public:
	/**
	 * Create an empty instance of a string buffer.
	 */
	inline stringbuf() : memstring(buffer, S) {};

	/**
	 * Create a string buffer from a null terminated string.
	 * @param text to place in object.
	 */
	inline stringbuf(const char *text) : memstring(buffer, S) {set(text);};

	/**
	 * Assign a string buffer from a null terminated string.
	 * @param text to assign to object.
	 */
	inline void operator=(const char *text)
		{set(text);};

	/**
	 * Assign a string buffer from another string object.
	 * @param object to assign from.
	 */
	inline void operator=(string& object)
		{set(object.c_str());};	
};

#if !defined(_MSWINDOWS_) && !defined(__QNX__)

/**
 * Convenience function for case insensitive null terminated string compare.
 * @param string1 to compare.
 * @param string2 to compare.
 * @return 0 if equal, > 0 if s2 > s1, < 0 if s2 < s1.
 */
extern "C" inline int stricmp(const char *string1, const char *string2)
	{return string::case_compare(string1, string2);}

/**
 * Convenience function for case insensitive null terminated string compare.
 * @param string1 to compare.
 * @param string2 to compare.
 * @param max size of string to compare.
 * @return 0 if equal, > 0 if s2 > s1, < 0 if s2 < s1.
 */
extern "C" inline int strnicmp(const char *string1, const char *string2, size_t max)
	{return string::case_compare(string1, string2, max);}

#endif

/**
 * A convenience type for string.
 */
typedef	string string_t;

/**
 * A convenience type when mixing std::string in old compilers that are bad
 * with namespaces...
 */
typedef string String;

/**
 * Compare two null terminated strings if equal.
 * @param s1 string to compare.
 * @param s2 string to compare.
 * @return true if equal.
 */
inline bool eq(char const *s1, char const *s2)
	{return String::equal(s1, s2);}

/**
 * Compare two null terminated strings if equal up to specified size.
 * @param s1 string to compare.
 * @param s2 string to compare.
 * @param size of string to compare.
 * @return true if equal.
 */
inline bool eq(char const *s1, char const *s2, size_t size)
	{return String::equal(s1, s2, size);}

/**
 * Compare two string objects if equal.
 * @param s1 string to compare.
 * @param s2 string to compare.
 * @return true if equal.
 */
inline bool eq(String &s1, String &s2)
	{return String::equal(s1.c_str(), s2.c_str());}

/**
 * Compare two null terminated strings if equal ignoring case.  This is
 * related to stricmp or gcc strcasecmp.
 * @param s1 string to compare.
 * @param s2 string to compare.
 * @return true if equal.
 */
inline bool ieq(char const *s1, char const *s2)
	{return String::case_equal(s1, s2);}

/**
 * Compare two null terminated strings if equal for a specified size 
 * ignoring case.  This is related to stricmp or gcc strcasecmp.
 * @param s1 string to compare.
 * @param s2 string to compare.
 * @param size of string to compare.
 * @return true if equal.
 */
inline bool ieq(char const *s1, char const *s2, size_t size)
	{return String::case_equal(s1, s2);}

/**
 * Compare two string objects if equal ignoring case.  This is
 * related to stricmp or gcc strcasecmp.
 * @param s1 string to compare.
 * @param s2 string to compare.
 * @return true if equal.
 */
inline bool ieq(String &s1, String &s2)
	{return String::case_equal(s1.c_str(), s2.c_str());}

END_NAMESPACE

#endif
