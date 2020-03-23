#include "pfc.h"

namespace pfc { namespace io { namespace path {

#ifdef _WINDOWS
static const string g_pathSeparators ("\\/|");
#else
static const string g_pathSeparators ("/");
#endif

string getFileName(string path) {
	t_size split = path.lastIndexOfAnyChar(g_pathSeparators);
	if (split == ~0) return path;
	else return path.subString(split+1);
}
string getFileNameWithoutExtension(string path) {
	string fn = getFileName(path);
	t_size split = fn.lastIndexOf('.');
	if (split == ~0) return fn;
	else return fn.subString(0,split);
}
string getFileExtension(string path) {
	string fn = getFileName(path);
	t_size split = fn.lastIndexOf('.');
	if (split == ~0) return "";
	else return fn.subString(split);
}
string getDirectory(string filePath) {return getParent(filePath);}

string getParent(string filePath) {
	t_size split = filePath.lastIndexOfAnyChar(g_pathSeparators);
	if (split == ~0) return "";
#ifdef _WINDOWS
	if (split > 0 && getIllegalNameChars().contains(filePath[split-1])) {
		if (split + 1 < filePath.length()) return filePath.subString(0,split+1);
		else return "";
	}
#endif
	return filePath.subString(0,split);
}
string combine(string basePath,string fileName) {
	if (basePath.length() > 0) {
		if (!isSeparator(basePath.lastChar())) {
			basePath += getDefaultSeparator();
		}
		return basePath + fileName;
	} else {
		//todo?
		return fileName;
	}
}

bool isSeparator(char c) {
	return g_pathSeparators.indexOf(c) != ~0;
}
string getSeparators() {
	return g_pathSeparators;
}

static string replaceIllegalChar(char c) {
	switch(c) {
		case '*':
			return "x";
		case '\"':
			return "\'\'";
		case ':':
		case '/':
		case '\\':
			return "-";
		default:
			return "_";
	}
}
string replaceIllegalPathChars(string fn) {
	string illegal = getIllegalNameChars();
	string separators = getSeparators();
	string_formatter output;
	for(t_size walk = 0; walk < fn.length(); ++walk) {
		const char c = fn[walk];
		if (separators.contains(c)) {
			output.add_byte(getDefaultSeparator());
		} else if (string::isNonTextChar(c) || illegal.contains(c)) {
			string replacement = replaceIllegalChar(c);
			if (replacement.containsAnyChar(illegal)) /*per-OS weirdness security*/ replacement = "_";
			output << replacement.ptr();
		} else {
			output.add_byte(c);
		}
	}
	return output.toString();
}

string replaceIllegalNameChars(string fn, bool allowWC) {
	const string illegal = getIllegalNameChars(allowWC);
	string_formatter output;
	for(t_size walk = 0; walk < fn.length(); ++walk) {
		const char c = fn[walk];
		if (string::isNonTextChar(c) || illegal.contains(c)) {
			string replacement = replaceIllegalChar(c);
			if (replacement.containsAnyChar(illegal)) /*per-OS weirdness security*/ replacement = "_";
			output << replacement.ptr();
		} else {
			output.add_byte(c);
		}
	}
	return output.toString();
}

bool isInsideDirectory(pfc::string directory, pfc::string inside) {
	//not very efficient
	string walk = inside;
	for(;;) {
		walk = getParent(walk);
		if (walk == "") return false;
		if (equals(directory,walk)) return true;
	}
}
bool isDirectoryRoot(string path) {
	return getParent(path).isEmpty();
}
//OS-dependant part starts here


char getDefaultSeparator() {
#ifdef _WINDOWS
	return '\\';
#else
    return '/';
#endif
}

static const string g_illegalNameChars(g_pathSeparators
#ifdef _WINDOWS
									   + ":<>*?\""
#else
                                       + "*?"
#endif
									   );
    
static const string g_illegalNameChars_noWC(g_pathSeparators
#ifdef _WINDOWS
									   + ":<>?\""
#endif
									   );
string getIllegalNameChars(bool allowWC) {
	return allowWC ? g_illegalNameChars_noWC : g_illegalNameChars;
}

#ifdef _WINDOWS
static bool isIllegalTrailingChar(char c) {
	switch (c) {
	case ' ':
	case '.':
	case '?':
		return true;
	default:
		return false;
	}
}
static const char * const specialIllegalNames[] = {
	"con", "aux", "lst", "prn", "nul", "eof", "inp", "out"
};

enum { maxPathComponent = 255 };
static size_t safeTruncat( const char * str, size_t maxLen ) {
	size_t i = 0;
	size_t ret = 0;
	for( ; i < maxLen; ++ i ) {
		auto d = pfc::utf8_char_len( str + ret );
		if ( d == 0 ) break;
		ret += d;
	}
	return ret;
}

static size_t utf8_length( const char * str ) {
	size_t ret = 0;
	for (; ++ret;) {
		size_t d = pfc::utf8_char_len( str );
		if ( d == 0 ) break;
		str += d;
	}
	return ret;
}
static string truncatePathComponent( string name, bool preserveExt ) {
	
	if (name.length() <= maxPathComponent) return name;
	if (preserveExt) {
		auto dot = name.lastIndexOf('.');
		if (dot != pfc_infinite) {
			const auto ext = name.subString(dot);
			const auto extLen = utf8_length( ext.c_str() );
			if (extLen < maxPathComponent) {
				auto lim = maxPathComponent - extLen;
				lim = safeTruncat( name.c_str(), lim );
				if (lim < dot) {
					return name.subString(0, lim) + ext;
				}
			}
		}
	}

	size_t truncat = safeTruncat( name.c_str(), maxPathComponent );
	return name.subString(0, truncat);
}

#endif
static string trailingSanity(string name, bool preserveExt) {
	t_size end = name.length();
	if (preserveExt) {
		size_t offset = pfc::string_find_last(name.c_str(), '.');
		if (offset < end) end = offset;
	}
	const size_t endEx = end;
	while (end > 0) {
		if (!isIllegalTrailingChar(name[end - 1])) break;
		--end;
	}
	t_size begin = 0;
	while (begin < end) {
		if (!isIllegalTrailingChar(name[begin])) break;
		++begin;
	}
	if (end < endEx || begin > 0) {
		name = name.subString(begin, end - begin) + name.subString(endEx);
	}
	return name;
}
string validateFileName(string name, bool allowWC, bool preserveExt) {
	if (!allowWC) { // special fix for filenames that consist only of question marks
		size_t end = name.length();
		if (preserveExt) {
			size_t offset = pfc::string_find_last(name.c_str(), '.');
			if (offset < end) end = offset;
		}
		bool unnamed = true;
		for (size_t walk = 0; walk < end; ++walk) {
			if (name[walk] != '?') unnamed = false;
		}
		if (unnamed) {
			name = string("[unnamed]") + name.subString(end);
		}
	}
#ifdef _WINDOWS
	if (name.length() > 0 && !allowWC) {
		name = trailingSanity(name, preserveExt);
	}

	name = replaceIllegalNameChars(name, allowWC);

	name = truncatePathComponent(name, preserveExt);
	
	for( unsigned w = 0; w < _countof(specialIllegalNames); ++w ) {
		if (pfc::stringEqualsI_ascii( name.c_str(), specialIllegalNames[w] ) ) {
			name += "-";
			break;
		}
	}

	if (name.isEmpty()) name = "_";
	return name;
#else
	return replaceIllegalNameChars(name);
#endif
}

}}}
