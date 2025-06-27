/* eKilo -- eKilo is a lightweight terminal text editor based on the original 
 *          kilo editor with enhanced functions. Its peculiarity is that it can
 *          run on any hardware, maintaining a high-performance and useful style.
 *          It installs in a minute, if you want to take a look I would be happy, 
 *          having found a utility myself. Any advice is highly appreciated.
 *          
 *      
 * Created by Antonio Foti - 2025, Italy.
 * -----------------------------------------------------------------------
 *
 * Copyright (C) 2016 Salvatore Sanfilippo <antirez at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define EKILO_VERSION "1.0.0"

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#endif

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>

/* Syntax highlight types */
#define HL_NORMAL 0
#define HL_NONPRINT 1
#define HL_COMMENT 2   /* Single line comment. */
#define HL_MLCOMMENT 3 /* Multi-line comment. */
#define HL_KEYWORD1 4
#define HL_KEYWORD2 5
#define HL_STRING 6
#define HL_NUMBER 7
#define HL_MATCH 8      /* Search match. */

#define HL_HIGHLIGHT_STRINGS (1<<0)
#define HL_HIGHLIGHT_NUMBERS (1<<1)

struct editorSyntax {
    char **filematch;
    char **keywords;
    char singleline_comment_start[3];
    char multiline_comment_start[3];
    char multiline_comment_end[3];
    int flags;
};

/* This structure represents a single line of the file we are editing. */
typedef struct erow {
    int idx;            /* Row index in the file, zero-based. */
    int size;           /* Size of the row, excluding the null term. */
    int rsize;          /* Size of the rendered row. */
    char *chars;        /* Row content. */
    char *render;       /* Row content "rendered" for screen (for TABs). */
    unsigned char *hl;  /* Syntax highlight type for each character in render.*/
    int hl_oc;          /* Row had open comment at end in last syntax highlight
                           check. */
} erow;

typedef struct hlcolor {
    int r,g,b;
} hlcolor;

struct editorConfig {
    int cx,cy;      			/* Cursor x and y position in characters */
    int rowoff;     			/* Offset of row displayed. */
    int coloff;     			/* Offset of column displayed. */
    int screenrows;			/* Number of rows that we can show */
    int screencols; 			/* Number of cols that we can show */
    int numrows;    			/* Number of rows */
    int rawmode;    			/* Is terminal raw mode enabled? */
    erow *row;      			/* Rows */
    int dirty;      			/* File modified but not saved. */
    int paste_mode; 			/* If 1, we're in paste mode - disable autocomplete */
    int last_key;			/* Last key pressed by user */
    char *filename;			/* Currently open filename */
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;    	/* Current syntax highlight, or NULL. */
    struct timeval last_char_time; 	/* Time of last char for paste detection */
    int should_quit;			/* Flag to indicate editor should quit gracefully */
};

static struct editorConfig E;

enum KEY_ACTION{
        KEY_NULL = 0,       /* NULL */
        CTRL_C = 3,         /* Ctrl-c */
        CTRL_D = 4,         /* Ctrl-d */
        CTRL_F = 6,         /* Ctrl-f */
        CTRL_H = 8,         /* Ctrl-h */
        TAB = 9,            /* Tab */
        CTRL_L = 12,        /* Ctrl+l */
        ENTER = 13,         /* Enter */
        CTRL_Q = 17,        /* Ctrl-q */
        CTRL_S = 19,        /* Ctrl-s */
        CTRL_U = 21,        /* Ctrl-u */
        ESC = 27,           /* Escape */
        BACKSPACE =  127,   /* Backspace */
        /* The following are just soft codes, not really reported by the
         * terminal directly. */
        ARROW_LEFT = 1000,
        ARROW_RIGHT,
        ARROW_UP,
        ARROW_DOWN,
        DEL_KEY,
        HOME_KEY,
        END_KEY,
        PAGE_UP,
        PAGE_DOWN
};

void editorSetStatusMessage(const char *fmt, ...);

/* =========================== Syntax highlights DB =========================
 *
 * In order to add a new syntax, define two arrays with a list of file name
 * matches and keywords. The file name matches are used in order to match
 * a given syntax with a given file name: if a match pattern starts with a
 * dot, it is matched as the last past of the filename, for example ".c".
 * Otherwise the pattern is just searched inside the filenme, like "Makefile").
 *
 * The list of keywords to highlight is just a list of words, however if they
 * a trailing '|' character is added at the end, they are highlighted in
 * a different color, so that you can have two different sets of keywords.
 *
 * Finally add a stanza in the HLDB global variable with two two arrays
 * of strings, and a set of flags in order to enable highlighting of
 * comments and numbers.
 *
 * The characters for single and multi line comments must be exactly two
 * and must be provided as well (see the C language example).
 *
 * There is no support to highlight patterns currently. */

/* C / C++ */
char *C_HL_extensions[] = {".c",".h",".cpp",".hpp",".cc",".cxx",".c++",".hxx",".h++",NULL};
char *C_HL_keywords[] = {
	/* C Keywords */
	"auto","break","case","continue","default","do","else","enum",
	"extern","for","goto","if","register","return","sizeof","static",
	"struct","switch","typedef","union","volatile","while","NULL",

	/* C++ Keywords */
	"alignas","alignof","and","and_eq","asm","bitand","bitor","class",
	"compl","constexpr","const_cast","deltype","delete","dynamic_cast",
	"explicit","export","false","friend","inline","mutable","namespace",
	"new","noexcept","not","not_eq","nullptr","operator","or","or_eq",
	"private","protected","public","reinterpret_cast","static_assert",
	"static_cast","template","this","thread_local","throw","true","try",
	"typeid","typename","virtual","xor","xor_eq",

	/* C types */
        "int|","long|","double|","float|","char|","unsigned|","signed|",
        "void|","short|","auto|","const|","bool|",NULL
};

/* Python */
char *PYTHON_HL_extensions[] = {".py",".pyw",".pyi",".pyx",NULL};
char *PYTHON_HL_keywords[] = {
	/* Python Keywords */
	"and","as","assert","break","class","continue","def","del",
	"elif","else","except","exec","finally","for","from","global",
	"if","import","in","is","lambda","not","or","pass","print",
	"raise","return","try","while","with","yield","async","await",
	"nonlocal","True","False","None",

	/* Python Built-ins */
	"abs|","all|","any|","bin|","bool|","bytearray|","bytes|","callable|",
	"chr|","classmethod|","compile|","complex|","delattr|","dict|","dir|",
	"divmod|","enumerate|","eval|","exec|","filter|","float|","format|",
	"frozenset|","getattr|","globals|","hasattr|","hash|","help|","hex|",
	"id|","input|","int|","isinstance|","issubclass|","iter|","len|",
	"list|","locals|","map|","max|","memoryview|","min|","next|","object|",
	"oct|","open|","ord|","pow|","property|","range|","repr|","reversed|",
	"round|","set|","setattr|","slice|","sorted|","staticmethod|","str|",
	"sum|","super|","tuple|","type|","vars|","zip|","self|","cls|",NULL
};

/* Java */
char *JAVA_HL_extensions[] = {".java",".class",NULL};
char *JAVA_HL_keywords[] = {
	/* Java Keywords */
	"abstract","assert","boolean","break","byte","case","catch","char",
	"class","const","continue","default","do","double","else","enum",
	"extends","final","finally","float","for","goto","if","implements",
	"import","instanceof","int","interface","long","native","new","package",
	"private","protected","public","return","short","static","strictfp",
	"super","switch","synchronized","this","throw","throws","transient",
	"try","void","volatile","while","true","false","null",

	/* Java Types and Common Classes */
	"String|","Object|","Class|","System|","Thread|","Runnable|",
	"Exception|","RuntimeException|","ArrayList|","HashMap|","List|",
	"Map|","Set|","Collection|","Iterator|","Comparable|","Serializable|",NULL
};

/* JavaScript */
char *JS_HL_extensions[] = {".js",".jsx",".mjs",".cjs",NULL};
char *JS_HL_keywords[] = {
	/* JavaScript Keywords */
	"break","case","catch","class","const","continue","debugger","default",
	"delete","do","else","export","extends","finally","for","function",
	"if","import","in","instanceof","let","new","return","super","switch",
	"this","throw","try","typeof","var","void","while","with","yield",
	"async","await","of","true","false","null","undefined",

	/* JavaScript Built-ins */
	"Array|","Object|","String|","Number|","Boolean|","Date|","Math|",
	"RegExp|","Error|","JSON|","console|","window|","document|","setTimeout|",
	"setInterval|","clearTimeout|","clearInterval|","parseInt|","parseFloat|",
	"isNaN|","isFinite|","encodeURI|","decodeURI|","Promise|","Map|","Set|",
	"WeakMap|","WeakSet|","Symbol|","Proxy|","Reflect|","Generator|",NULL
};

/* TypeScript */
char *TS_HL_extensions[] = {".ts",".tsx",".d.ts",NULL};
char *TS_HL_keywords[] = {
	/* TypeScript Keywords (includes JavaScript) */
	"break","case","catch","class","const","continue","debugger","default",
	"delete","do","else","export","extends","finally","for","function",
	"if","import","in","instanceof","let","new","return","super","switch",
	"this","throw","try","typeof","var","void","while","with","yield",
	"async","await","of","true","false","null","undefined",
	
	/* TypeScript Specific */
	"interface","type","enum","namespace","module","declare","abstract",
	"implements","private","protected","public","readonly","static",
	"get","set","as","keyof","infer","is","asserts",

	/* TypeScript Types */
	"string|","number|","boolean|","object|","any|","unknown|","never|",
	"void|","bigint|","symbol|","Array|","Promise|","Record|","Partial|",
	"Required|","Pick|","Omit|","Exclude|","Extract|","NonNullable|",NULL
};

/* C# */
char *CSHARP_HL_extensions[] = {".cs",".csx",NULL};
char *CSHARP_HL_keywords[] = {
	/* C# Keywords */
	"abstract","as","base","bool","break","byte","case","catch","char",
	"checked","class","const","continue","decimal","default","delegate",
	"do","double","else","enum","event","explicit","extern","false",
	"finally","fixed","float","for","foreach","goto","if","implicit",
	"in","int","interface","internal","is","lock","long","namespace",
	"new","null","object","operator","out","override","params","private",
	"protected","public","readonly","ref","return","sbyte","sealed",
	"short","sizeof","stackalloc","static","string","struct","switch",
	"this","throw","true","try","typeof","uint","ulong","unchecked",
	"unsafe","ushort","using","virtual","void","volatile","while",
	"async","await","var","dynamic","yield","where","when","nameof",

	/* C# Types */
	"String|","Object|","Int32|","Boolean|","Double|","DateTime|","List|",
	"Dictionary|","Array|","IEnumerable|","ICollection|","IList|","Task|",
	"Exception|","ArgumentException|","NullReferenceException|",NULL
};

/* PHP */
char *PHP_HL_extensions[] = {".php",".phtml",".php3",".php4",".php5",".phps",NULL};
char *PHP_HL_keywords[] = {
	/* PHP Keywords */
	"abstract","and","array","as","break","callable","case","catch",
	"class","clone","const","continue","declare","default","die","do",
	"echo","else","elseif","empty","enddeclare","endfor","endforeach",
	"endif","endswitch","endwhile","eval","exit","extends","final",
	"finally","for","foreach","function","global","goto","if","implements",
	"include","include_once","instanceof","insteadof","interface","isset",
	"list","namespace","new","or","print","private","protected","public",
	"require","require_once","return","static","switch","throw","trait",
	"try","unset","use","var","while","xor","yield","true","false","null",

	/* PHP Built-ins */
	"$_GET|","$_POST|","$_SESSION|","$_COOKIE|","$_SERVER|","$_FILES|",
	"$_ENV|","$_REQUEST|","$GLOBALS|","strlen|","substr|","strpos|",
	"explode|","implode|","array_merge|","array_push|","array_pop|",
	"count|","sizeof|","is_array|","is_string|","is_numeric|","empty|",
	"isset|","unset|","die|","exit|","echo|","print|","var_dump|",NULL
};

/* Ruby */
char *RUBY_HL_extensions[] = {".rb",".rbw",".rake",".gemspec",NULL};
char *RUBY_HL_keywords[] = {
	/* Ruby Keywords */
	"alias","and","begin","break","case","class","def","defined","do",
	"else","elsif","end","ensure","false","for","if","in","module",
	"next","nil","not","or","redo","rescue","retry","return","self",
	"super","then","true","undef","unless","until","when","while","yield",
	"require","include","extend","attr_reader","attr_writer","attr_accessor",

	/* Ruby Built-ins */
	"puts|","print|","p|","gets|","chomp|","strip|","length|","size|",
	"empty|","nil|","class|","new|","initialize|","to_s|","to_i|","to_f|",
	"to_a|","each|","map|","select|","reject|","find|","inject|","reduce|",
	"Array|","Hash|","String|","Integer|","Float|","Symbol|","Proc|",
	"Lambda|","Method|","Class|","Module|","Object|","Kernel|",NULL
};

/* Swift */
char *SWIFT_HL_extensions[] = {".swift",NULL};
char *SWIFT_HL_keywords[] = {
	/* Swift Keywords */
	"associatedtype","class","deinit","enum","extension","fileprivate","func",
	"import","init","inout","internal","let","open","operator","private",
	"protocol","public","static","struct","subscript","typealias","var",
	"break","case","continue","default","defer","do","else","fallthrough",
	"for","guard","if","in","repeat","return","switch","where","while",
	"as","catch","false","is","nil","rethrows","super","self","Self",
	"throw","throws","true","try","async","await","some","any",

	/* Swift Types */
	"Int|","Double|","Float|","Bool|","String|","Character|","Array|",
	"Dictionary|","Set|","Optional|","Result|","Error|","AnyObject|",
	"AnyClass|","Protocol|","Codable|","Hashable|","Equatable|",
	"Comparable|","Collection|","Sequence|",NULL
};

/* SQL */
char *SQL_HL_extensions[] = {".sql",".ddl",".dml",NULL};
char *SQL_HL_keywords[] = {
	/* SQL Keywords */
	"SELECT","FROM","WHERE","INSERT","UPDATE","DELETE","CREATE","DROP",
	"ALTER","TABLE","INDEX","VIEW","DATABASE","SCHEMA","COLUMN","PRIMARY",
	"FOREIGN","KEY","REFERENCES","CONSTRAINT","UNIQUE","NOT","NULL","DEFAULT",
	"AUTO_INCREMENT","IDENTITY","SERIAL","BOOLEAN","TINYINT","SMALLINT",
	"MEDIUMINT","INT","INTEGER","BIGINT","DECIMAL","NUMERIC","FLOAT","DOUBLE",
	"REAL","BIT","DATE","TIME","DATETIME","TIMESTAMP","YEAR","CHAR","VARCHAR",
	"BINARY","VARBINARY","TINYBLOB","BLOB","MEDIUMBLOB","LONGBLOB","TINYTEXT",
	"TEXT","MEDIUMTEXT","LONGTEXT","ENUM","SET","JSON","GEOMETRY","POINT",
	"LINESTRING","POLYGON","MULTIPOINT","MULTILINESTRING","MULTIPOLYGON",
	"GEOMETRYCOLLECTION","AND","OR","IN","BETWEEN","LIKE","IS","EXISTS",
	"ANY","ALL","SOME","UNION","INTERSECT","EXCEPT","INNER","LEFT","RIGHT",
	"FULL","OUTER","JOIN","ON","USING","GROUP","BY","HAVING","ORDER","ASC",
	"DESC","LIMIT","OFFSET","DISTINCT","AS","CASE","WHEN","THEN","ELSE","END",
	"IF","IFNULL","ISNULL","COALESCE","NULLIF","CAST","CONVERT","SUBSTRING",
	"LENGTH","UPPER","LOWER","TRIM","LTRIM","RTRIM","REPLACE","CONCAT",
	"CURRENT_DATE","CURRENT_TIME","CURRENT_TIMESTAMP","NOW","COUNT","SUM",
	"AVG","MIN","MAX","STDDEV","VARIANCE","BEGIN","COMMIT","ROLLBACK",
	"TRANSACTION","SAVEPOINT","GRANT","REVOKE","LOCK","UNLOCK",

	/* SQL Functions and Operators */
	"TRUE|","FALSE|","UNKNOWN|",NULL
};

/* Rust */
char *RUST_HL_extensions[] = {".rs",".rlib",NULL};
char *RUST_HL_keywords[] = {
	/* Rust Keywords */
	"as","async","await","break","const","continue","crate","dyn","else",
	"enum","extern","false","fn","for","if","impl","in","let","loop",
	"match","mod","move","mut","pub","ref","return","self","Self","static",
	"struct","super","trait","true","type","unsafe","use","where","while",
	"abstract","become","box","do","final","macro","override","priv",
	"typeof","unsized","virtual","yield","try","union","catch","default",

	/* Rust Types */
	"i8|","i16|","i32|","i64|","i128|","isize|","u8|","u16|","u32|","u64|",
	"u128|","usize|","f32|","f64|","bool|","char|","str|","String|","Vec|",
	"HashMap|","HashSet|","BTreeMap|","BTreeSet|","Option|","Result|","Box|",
	"Rc|","Arc|","RefCell|","Cell|","Mutex|","RwLock|","thread|","Clone|",
	"Copy|","Send|","Sync|","Drop|","Display|","Debug|","Default|","PartialEq|",
	"Eq|","PartialOrd|","Ord|","Hash|","Iterator|","IntoIterator|",NULL
};

/* Dart */
char *DART_HL_extensions[] = {".dart",NULL};
char *DART_HL_keywords[] = {
	/* Dart Keywords */
	"abstract","as","assert","async","await","break","case","catch","class",
	"const","continue","covariant","default","deferred","do","dynamic","else",
	"enum","export","extends","extension","external","factory","false","final",
	"finally","for","Function","get","hide","if","implements","import","in",
	"interface","is","late","library","mixin","new","null","on","operator",
	"part","required","rethrow","return","set","show","static","super","switch",
	"sync","this","throw","true","try","typedef","var","void","while","with",
	"yield",

	/* Dart Types */
	"int|","double|","num|","String|","bool|","List|","Map|","Set|","Object|",
	"dynamic|","var|","void|","Future|","Stream|","Iterable|","Iterator|",
	"Comparable|","Duration|","DateTime|","Uri|","RegExp|","StringBuffer|",
	"Symbol|","Type|","Function|","Null|",NULL
};

/* Here we define an array of syntax highlights by extensions, keywords,
 * comments delimiters and flags. */
struct editorSyntax HLDB[] = {
    {
        /* C / C++ */
        C_HL_extensions,
        C_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Python */
        PYTHON_HL_extensions,
        PYTHON_HL_keywords,
        "#","","",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Java */
        JAVA_HL_extensions,
        JAVA_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* JavaScript */
        JS_HL_extensions,
        JS_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* TypeScript */
        TS_HL_extensions,
        TS_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* C# */
        CSHARP_HL_extensions,
        CSHARP_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* PHP */
        PHP_HL_extensions,
        PHP_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Ruby */
        RUBY_HL_extensions,
        RUBY_HL_keywords,
        "#","","",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Swift */
        SWIFT_HL_extensions,
        SWIFT_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* SQL */
        SQL_HL_extensions,
        SQL_HL_keywords,
        "--","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Rust */
        RUST_HL_extensions,
        RUST_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Dart */
        DART_HL_extensions,
        DART_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    }
};

#define HLDB_ENTRIES (sizeof(HLDB)/sizeof(HLDB[0]))

/* Function prototypes */
void editorInsertChar(int c);
void editorMoveCursor(int key);

/* ============================ Autocompletion ============================= */

/* Define pairs of characters that should be autocompleted */
struct autopair {
    int open_char;       /* Opening character (like '{') */
    int close_char;      /* Closing character (like '}') */
};

/* Array of autocomplete pairs. Can be extended with more character pairs. */
struct autopair autopairs[] = {
    {'{', '}'},
    {'[', ']'},
    {'(', ')'},
    {'\"', '\"'},
    {'\'', '\''},
    {'`', '`'},
    {'<', '>'},
    /* Add more pairs here if needed */
};

/* Find the matching closing character for the given opening character.
 * Returns the closing character if found, or 0 if no match exists. */
int editorFindCloseChar(int open_char) {
    for (size_t i = 0; i < sizeof(autopairs)/sizeof(autopairs[0]); i++) {
        if (autopairs[i].open_char == open_char) {
            return autopairs[i].close_char;
        }
    }
    return 0;
}

/* Handle character insertion with potential autocompletion.
 * Checks if the typed character has a closing pair and inserts it automatically
 * when appropriate. */
void editorInsertCharAutoComplete(int c) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    
    /* Check if we're at end of line or the next character is whitespace/symbol */
    int at_end = (!row || filecol >= row->size);
    int next_char_space = at_end || isspace(row->chars[filecol]) || 
                          strchr(",.()+-/*=~%[];{}", row->chars[filecol]);

    /* Find closing character if this is an opening bracket/quote */
    int close_char = editorFindCloseChar(c);
    
    /* Insert the character first */
    editorInsertChar(c);

    /* Skip autocompletion during paste operations */
    if (E.paste_mode) return;
    
    /* If this is a bracket/quote and we're either at end of line or 
     * next character is whitespace/symbol, insert the closing character */
    if (close_char && next_char_space) {
        editorInsertChar(close_char);
        
        /* Move cursor back between the pair */
        editorMoveCursor(ARROW_LEFT);
    }
}


/* ======================= Low level terminal handling ====================== */

static struct termios orig_termios; /* In order to restore at exit.*/

void disableRawMode(int fd) {
    /* Don't even check the return value as it's too late. */
    if (E.rawmode) {
        tcsetattr(fd,TCSAFLUSH,&orig_termios);
        E.rawmode = 0;
    }
}

/* Called at exit to avoid remaining in raw mode. */
void editorAtExit(void) {
    disableRawMode(STDIN_FILENO);
 /* Clear screen and position cursor at top-left */
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

/* Raw mode: 1960 magic shit. */
int enableRawMode(int fd) {
    struct termios raw;

    if (E.rawmode) return 0; /* Already enabled. */
    if (!isatty(STDIN_FILENO)) goto fatal;
    atexit(editorAtExit);
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[VMIN] = 0; /* Return each byte, or zero for timeout. */
    raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    E.rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int editorReadKey(int fd) {
    int nread;
    char c, seq[3];
    while ((nread = read(fd,&c,1)) == 0);
    if (nread == -1) exit(1);

    while(1) {
        switch(c) {
        case ESC:    /* escape sequence */
            /* If this is just an ESC, we'll timeout here. */
            if (read(fd,seq,1) == 0) return ESC;
            if (read(fd,seq+1,1) == 0) return ESC;

            /* ESC [ sequences. */
            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    /* Extended escape, read additional byte. */
                    if (read(fd,seq+2,1) == 0) return ESC;
                    if (seq[2] == '~') {
                        switch(seq[1]) {
                        case '3': return DEL_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        }
                    }
                } else {
                    switch(seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                    }
                }
            }

            /* ESC O sequences. */
            else if (seq[0] == 'O') {
                switch(seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
                }
            }
            break;
        default:
            return c;
        }
    }
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int getCursorPosition(int ifd, int ofd, int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4) return -1;

    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf)-1) {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    /* Parse it. */
    if (buf[0] != ESC || buf[1] != '[') return -1;
    if (sscanf(buf+2,"%d;%d",rows,cols) != 2) return -1;
    return 0;
}

/* Try to get the number of columns in the current terminal. If the ioctl()
 * call fails the function will try to query the terminal itself.
 * Returns 0 on success, -1 on error. */
int getWindowSize(int ifd, int ofd, int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        int orig_row, orig_col, retval;

        /* Get the initial position so we can restore it later. */
        retval = getCursorPosition(ifd,ofd,&orig_row,&orig_col);
        if (retval == -1) goto failed;

        /* Go to right/bottom margin and get position. */
        if (write(ofd,"\x1b[999C\x1b[999B",12) != 12) goto failed;
        retval = getCursorPosition(ifd,ofd,rows,cols);
        if (retval == -1) goto failed;

        /* Restore position. */
        char seq[32];
        snprintf(seq,32,"\x1b[%d;%dH",orig_row,orig_col);
        if (write(ofd,seq,strlen(seq)) == -1) {
            /* Can't recover... */
        }
        return 0;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }

failed:
    return -1;
}

/* ====================== Syntax highlight color scheme  ==================== */

int is_separator(int c) {
    return c == '\0' || isspace(c) || strchr(",.()+-/*=~%[];",c) != NULL;
}

/* Return true if the specified row last char is part of a multi line comment
 * that starts at this row or at one before, and does not end at the end
 * of the row but spawns to the next row. */
int editorRowHasOpenComment(erow *row) {
    if (row->hl && row->rsize && row->hl[row->rsize-1] == HL_MLCOMMENT &&
        (row->rsize < 2 || (row->render[row->rsize-2] != '*' ||
                            row->render[row->rsize-1] != '/'))) return 1;
    return 0;
}

/* Set every byte of row->hl (that corresponds to every character in the line)
 * to the right syntax highlight type (HL_* defines). */
void editorUpdateSyntax(erow *row) {
    row->hl = realloc(row->hl,row->rsize);
    memset(row->hl,HL_NORMAL,row->rsize);

    if (E.syntax == NULL) return; /* No syntax, everything is HL_NORMAL. */

    int i, prev_sep, in_string, in_comment;
    char *p;
    char **keywords = E.syntax->keywords;
    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;

    /* Point to the first non-space char. */
    p = row->render;
    i = 0; /* Current char offset */
    while(*p && isspace(*p)) {
        p++;
        i++;
    }
    prev_sep = 1; /* Tell the parser if 'i' points to start of word. */
    in_string = 0; /* Are we inside "" or '' ? */
    in_comment = 0; /* Are we inside multi-line comment? */

    /* If the previous line has an open comment, this line starts
     * with an open comment state. */
    if (row->idx > 0 && editorRowHasOpenComment(&E.row[row->idx-1]))
        in_comment = 1;

    while(*p) {
       /* Handle single line comments (both single and double character markers) */
       if (prev_sep && *p == scs[0]) {
           /* Check if this is really a comment start (next char matches or scs is single char) */
           if (scs[1] == '\0' || *(p+1) == scs[1]) {
               /* From here to end is a comment */
               memset(row->hl+i,HL_COMMENT,row->size-i);
               return;
           }
        }

        /* Handle multi line comments. */
        if (in_comment) {
            row->hl[i] = HL_MLCOMMENT;
            if (*p == mce[0] && *(p+1) == mce[1]) {
                row->hl[i+1] = HL_MLCOMMENT;
                p += 2; i += 2;
                in_comment = 0;
                prev_sep = 1;
                continue;
            } else {
                prev_sep = 0;
                p++; i++;
                continue;
            }
        } else if (*p == mcs[0] && *(p+1) == mcs[1]) {
            row->hl[i] = HL_MLCOMMENT;
            row->hl[i+1] = HL_MLCOMMENT;
            p += 2; i += 2;
            in_comment = 1;
            prev_sep = 0;
            continue;
        }

        /* Handle "" and '' */
        if (in_string) {
            row->hl[i] = HL_STRING;
            if (*p == '\\') {
                row->hl[i+1] = HL_STRING;
                p += 2; i += 2;
                prev_sep = 0;
                continue;
            }
            if (*p == in_string) in_string = 0;
            p++; i++;
            continue;
        } else {
            if (*p == '"' || *p == '\'') {
                in_string = *p;
                row->hl[i] = HL_STRING;
                p++; i++;
                prev_sep = 0;
                continue;
            }
        }

        /* Handle non printable chars. */
        if (!isprint(*p)) {
            row->hl[i] = HL_NONPRINT;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        /* Handle numbers */
        if ((isdigit(*p) && (prev_sep || row->hl[i-1] == HL_NUMBER)) ||
            (*p == '.' && i >0 && row->hl[i-1] == HL_NUMBER)) {
            row->hl[i] = HL_NUMBER;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        /* Handle keywords and lib calls */
        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen-1] == '|';
                if (kw2) klen--;

                if (!memcmp(p,keywords[j],klen) &&
                    is_separator(*(p+klen)))
                {
                    /* Keyword */
                    memset(row->hl+i,kw2 ? HL_KEYWORD2 : HL_KEYWORD1,klen);
                    p += klen;
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue; /* We had a keyword match */
            }
        }

        /* Not special chars */
        prev_sep = is_separator(*p);
        p++; i++;
    }

    /* Propagate syntax change to the next row if the open commen
     * state changed. This may recursively affect all the following rows
     * in the file. */
    int oc = editorRowHasOpenComment(row);
    if (row->hl_oc != oc && row->idx+1 < E.numrows)
        editorUpdateSyntax(&E.row[row->idx+1]);
    row->hl_oc = oc;
}

/* Maps syntax highlight token types to terminal colors. */
int editorSyntaxToColor(int hl) {
    switch(hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT: return 36;     /* cyan */
    case HL_KEYWORD1: return 33;    /* yellow */
    case HL_KEYWORD2: return 32;    /* green */
    case HL_STRING: return 35;      /* magenta */
    case HL_NUMBER: return 31;      /* red */
    case HL_MATCH: return 34;      /* blu */
    default: return 37;             /* white */
    }
}

/* Select the syntax highlight scheme depending on the filename,
 * setting it in the global state E.syntax. */
void editorSelectSyntaxHighlight(char *filename) {
    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct editorSyntax *s = HLDB+j;
        unsigned int i = 0;
        while(s->filematch[i]) {
            char *p;
            int patlen = strlen(s->filematch[i]);
            if ((p = strstr(filename,s->filematch[i])) != NULL) {
                if (s->filematch[i][0] != '.' || p[patlen] == '\0') {
                    E.syntax = s;
                    return;
                }
            }
            i++;
        }
    }
}

/* ======================= Editor rows implementation ======================= */

/* Update the rendered version and the syntax highlight of a row. */
void editorUpdateRow(erow *row) {
    unsigned int tabs = 0, nonprint = 0;
    int j, idx;

   /* Create a version of the row we can directly print on the screen,
     * respecting tabs, substituting non printable characters with '?'. */
    free(row->render);
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == TAB) tabs++;

    unsigned long long allocsize =
        (unsigned long long) row->size + tabs*8 + nonprint*9 + 1;
    if (allocsize > UINT32_MAX) {
        printf("Some line of the edited file is too long for ekilo\n");
        exit(1);
    }

    row->render = malloc(row->size + tabs*8 + nonprint*9 + 1);
    idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == TAB) {
            row->render[idx++] = ' ';
            while((idx+1) % 8 != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->rsize = idx;
    row->render[idx] = '\0';

    /* Update the syntax highlighting attributes of the row. */
    editorUpdateSyntax(row);
}

/* Insert a row at the specified position, shifting the other rows on the bottom
 * if required. */
void editorInsertRow(int at, char *s, size_t len) {
    if (at > E.numrows) return;
    E.row = realloc(E.row,sizeof(erow)*(E.numrows+1));
    if (at != E.numrows) {
        memmove(E.row+at+1,E.row+at,sizeof(E.row[0])*(E.numrows-at));
        for (int j = at+1; j <= E.numrows; j++) E.row[j].idx++;
    }
    E.row[at].size = len;
    E.row[at].chars = malloc(len+1);
    memcpy(E.row[at].chars,s,len+1);
    E.row[at].hl = NULL;
    E.row[at].hl_oc = 0;
    E.row[at].render = NULL;
    E.row[at].rsize = 0;
    E.row[at].idx = at;
    editorUpdateRow(E.row+at);
    E.numrows++;
    E.dirty++;
}

/* Free row's heap allocated stuff. */
void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

/* Remove the row at the specified position, shifting the remainign on the
 * top. */
void editorDelRow(int at) {
    erow *row;

    if (at >= E.numrows) return;
    row = E.row+at;
    editorFreeRow(row);
    memmove(E.row+at,E.row+at+1,sizeof(E.row[0])*(E.numrows-at-1));
    for (int j = at; j < E.numrows-1; j++) E.row[j].idx++;
    E.numrows--;
    E.dirty++;
}

/* Turn the editor rows into a single heap-allocated string.
 * Returns the pointer to the heap-allocated string and populate the
 * integer pointed by 'buflen' with the size of the string, escluding
 * the final nulterm. */
char *editorRowsToString(int *buflen) {
    char *buf = NULL, *p;
    int totlen = 0;
    int j;

    /* Compute count of bytes */
    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size+1; /* +1 is for "\n" at end of every row */
    *buflen = totlen;
    totlen++; /* Also make space for nulterm */

    p = buf = malloc(totlen);
    for (j = 0; j < E.numrows; j++) {
        memcpy(p,E.row[j].chars,E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    *p = '\0';
    return buf;
}

/* Insert a character at the specified position in a row, moving the remaining
 * chars on the right if needed. */
void editorRowInsertChar(erow *row, int at, int c) {
    if (at > row->size) {
        /* Pad the string with spaces if the insert location is outside the
         * current length by more than a single character. */
        int padlen = at-row->size;
        /* In the next line +2 means: new char and null term. */
        row->chars = realloc(row->chars,row->size+padlen+2);
        memset(row->chars+row->size,' ',padlen);
        row->chars[row->size+padlen+1] = '\0';
        row->size += padlen+1;
    } else {
        /* If we are in the middle of the string just make space for 1 new
         * char plus the (already existing) null term. */
        row->chars = realloc(row->chars,row->size+2);
        memmove(row->chars+at+1,row->chars+at,row->size-at+1);
        row->size++;
    }
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

/* Append the string 's' at the end of a row */
void editorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars,row->size+len+1);
    memcpy(row->chars+row->size,s,len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

/* Delete the character at offset 'at' from the specified row. */
void editorRowDelChar(erow *row, int at) {
    if (row->size <= at) return;
    memmove(row->chars+at,row->chars+at+1,row->size-at);
    editorUpdateRow(row);
    row->size--;
    E.dirty++;
}

/* Insert the specified char at the current prompt position. */
void editorInsertChar(int c) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    /* If the row where the cursor is currently located does not exist in our
     * logical representaion of the file, add enough empty rows as needed. */
    if (!row) {
        while(E.numrows <= filerow)
            editorInsertRow(E.numrows,"",0);
    }
    row = &E.row[filerow];
    editorRowInsertChar(row,filecol,c);
    if (E.cx == E.screencols-1)
        E.coloff++;
    else
        E.cx++;
    E.dirty++;
}

/* Inserting a newline is slightly complex as we have to handle inserting a
 * newline in the middle of a line, splitting the line as needed. */
void editorInsertNewline(void) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row) {
        if (filerow == E.numrows) {
            editorInsertRow(filerow,"",0);
            goto fixcursor;
        }
        return;
    }
    /* If the cursor is over the current line size, we want to conceptually
     * think it's just over the last character. */
    if (filecol >= row->size) filecol = row->size;
    if (filecol == 0) {
        editorInsertRow(filerow,"",0);
    } else {
        /* We are in the middle of a line. Split it between two rows. */
        editorInsertRow(filerow+1,row->chars+filecol,row->size-filecol);
        row = &E.row[filerow];
        row->chars[filecol] = '\0';
        row->size = filecol;
        editorUpdateRow(row);
    }
fixcursor:
    if (E.cy == E.screenrows-1) {
        E.rowoff++;
    } else {
        E.cy++;
    }
    E.cx = 0;
    E.coloff = 0;
}

/* Delete the char at the current prompt position. */
void editorDelChar(void) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    /* Handle CTRL-H (delete entire line) */
    if (E.last_key == CTRL_H && row) {
        /* Delete the current line */
        editorDelRow(filerow);
        /* Adjust cursor position */
        if (E.cy > 0) {
            E.cy--;
        } else if (E.rowoff > 0) {
            E.rowoff--;
        }
        E.cx = 0;
        E.coloff = 0;
        E.dirty++;
        return;
    }

    if (!row || (filecol == 0 && filerow == 0)) return;
    if (filecol == 0) {
        /* Handle the case of column 0, we need to move the current line
         * on the right of the previous one. */
        filecol = E.row[filerow-1].size;
        editorRowAppendString(&E.row[filerow-1],row->chars,row->size);
        editorDelRow(filerow);
        row = NULL;
        if (E.cy == 0)
            E.rowoff--;
        else
            E.cy--;
        E.cx = filecol;
        if (E.cx >= E.screencols) {
            int shift = (E.screencols-E.cx)+1;
            E.cx -= shift;
            E.coloff += shift;
        }
    } else {
        editorRowDelChar(row,filecol-1);
        if (E.cx == 0 && E.coloff)
            E.coloff--;
        else
            E.cx--;
    }
    if (row) editorUpdateRow(row);
    E.dirty++;
}

/* Load the specified program in the editor memory and returns 0 on success
 * or 1 on error. */
int editorOpen(char *filename) {
    FILE *fp;

    E.dirty = 0;
    free(E.filename);
    size_t fnlen = strlen(filename)+1;
    E.filename = malloc(fnlen);
    memcpy(E.filename,filename,fnlen);

    fp = fopen(filename,"r");
    if (!fp) {
        if (errno != ENOENT) {
            perror("Opening file");
            exit(1);
        }
        return 1;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while((linelen = getline(&line,&linecap,fp)) != -1) {
        if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
            line[--linelen] = '\0';
        editorInsertRow(E.numrows,line,linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
    return 0;
}

/* Save the current file on disk. Return 0 on success, 1 on error. */
int editorSave(void) {
    int len;
    char *buf = editorRowsToString(&len);
    int fd = open(E.filename,O_RDWR|O_CREAT,0644);
    if (fd == -1) goto writeerr;

    /* Use truncate + a single write(2) call in order to make saving
     * a bit safer, under the limits of what we can do in a small editor. */
    if (ftruncate(fd,len) == -1) goto writeerr;
    if (write(fd,buf,len) != len) goto writeerr;

    close(fd);
    free(buf);
    E.dirty = 0;
    editorSetStatusMessage("%d bytes written on disk", len);
    return 0;

writeerr:
    free(buf);
    if (fd != -1) close(fd);
    editorSetStatusMessage("Can't save! I/O error: %s",strerror(errno));
    return 1;
}

/* ============================= Terminal update ============================ */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL,0}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b,ab->len+len);

    if (new == NULL) return;
    memcpy(new+ab->len,s,len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'. */
void editorRefreshScreen(void) {
    int y;
    erow *r;
    char buf[32];
    struct abuf ab = ABUF_INIT;

    abAppend(&ab,"\x1b[?25l",6); /* Hide cursor. */
    abAppend(&ab,"\x1b[H",3); /* Go home. */
    for (y = 0; y < E.screenrows; y++) {
        int filerow = E.rowoff+y;

        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows/3) {
                char welcome[80];
                int welcomelen = snprintf(welcome,sizeof(welcome),
                    "eKilo editor -- verison %s\x1b[0K\r\n", EKILO_VERSION);
                int padding = (E.screencols-welcomelen)/2;
                if (padding) {
                    abAppend(&ab,"~",1);
                    padding--;
                }
                while(padding--) abAppend(&ab," ",1);
                abAppend(&ab,welcome,welcomelen);
            } else {
                abAppend(&ab,"~\x1b[0K\r\n",7);
            }
            continue;
        }

        r = &E.row[filerow];

        int len = r->rsize - E.coloff;
        int current_color = -1;
        if (len > 0) {
            if (len > E.screencols) len = E.screencols;
            char *c = r->render+E.coloff;
            unsigned char *hl = r->hl+E.coloff;
            int j;
            for (j = 0; j < len; j++) {
                if (hl[j] == HL_NONPRINT) {
                    char sym;
                    abAppend(&ab,"\x1b[7m",4);
                    if (c[j] <= 26)
                        sym = '@'+c[j];
                    else
                        sym = '?';
                    abAppend(&ab,&sym,1);
                    abAppend(&ab,"\x1b[0m",4);
                } else if (hl[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        abAppend(&ab,"\x1b[39m",5);
                        current_color = -1;
                    }
                    abAppend(&ab,c+j,1);
                } else {
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != current_color) {
                        char buf[16];
                        int clen = snprintf(buf,sizeof(buf),"\x1b[%dm",color);
                        current_color = color;
                        abAppend(&ab,buf,clen);
                    }
                    abAppend(&ab,c+j,1);
                }
            }
        }
        abAppend(&ab,"\x1b[39m",5);
        abAppend(&ab,"\x1b[0K",4);
        abAppend(&ab,"\r\n",2);
    }

    /* Create a two rows status. First row: */
    abAppend(&ab,"\x1b[0K",4);
    abAppend(&ab,"\x1b[7m",4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
        E.filename, E.numrows, E.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus),
        "%d/%d",E.rowoff+E.cy+1,E.numrows);
    if (len > E.screencols) len = E.screencols;
    abAppend(&ab,status,len);
    while(len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(&ab,rstatus,rlen);
            break;
        } else {
            abAppend(&ab," ",1);
            len++;
        }
    }
    abAppend(&ab,"\x1b[0m\r\n",6);

    /* Second row depends on E.statusmsg and the status message update time. */
    abAppend(&ab,"\x1b[0K",4);
    int msglen = strlen(E.statusmsg);
    if (msglen && time(NULL)-E.statusmsg_time < 5)
        abAppend(&ab,E.statusmsg,msglen <= E.screencols ? msglen : E.screencols);

    /* Put cursor at its current position. Note that the horizontal position
     * at which the cursor is displayed may be different compared to 'E.cx'
     * because of TABs. */
    int j;
    int cx = 1;
    int filerow = E.rowoff+E.cy;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    if (row) {
        for (j = E.coloff; j < (E.cx+E.coloff); j++) {
            if (j < row->size && row->chars[j] == TAB) cx += 7-((cx)%8);
            cx++;
        }
    }
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",E.cy+1,cx);
    abAppend(&ab,buf,strlen(buf));
    abAppend(&ab,"\x1b[?25h",6); /* Show cursor. */
    write(STDOUT_FILENO,ab.b,ab.len);
    abFree(&ab);
}

/* Set an editor status message for the second line of the status, at the
 * end of the screen. */
void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(E.statusmsg,sizeof(E.statusmsg),fmt,ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

/* =============================== Find mode ================================ */

#define EKILO_QUERY_LEN 256

void editorFind(int fd) {
    char query[EKILO_QUERY_LEN+1] = {0};
    int qlen = 0;
    int last_match = -1; /* Last line where a match was found. -1 for none. */
    int find_next = 0; /* if 1 search next, if -1 search prev. */
    int saved_hl_line = -1;  /* No saved HL */
    char *saved_hl = NULL;

#define FIND_RESTORE_HL do { \
    if (saved_hl) { \
        memcpy(E.row[saved_hl_line].hl,saved_hl, E.row[saved_hl_line].rsize); \
        free(saved_hl); \
        saved_hl = NULL; \
    } \
} while (0)

    /* Save the cursor position in order to restore it later. */
    int saved_cx = E.cx, saved_cy = E.cy;
    int saved_coloff = E.coloff, saved_rowoff = E.rowoff;

    while(1) {
        editorSetStatusMessage(
            "Search: %s (Use ESC/Arrows/Enter)", query);
        editorRefreshScreen();

        int c = editorReadKey(fd);
        if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
            if (qlen != 0) query[--qlen] = '\0';
            last_match = -1;
        } else if (c == ESC || c == ENTER) {
            if (c == ESC) {
                E.cx = saved_cx; E.cy = saved_cy;
                E.coloff = saved_coloff; E.rowoff = saved_rowoff;
            }
            FIND_RESTORE_HL;
            editorSetStatusMessage("");
            return;
        } else if (c == ARROW_RIGHT || c == ARROW_DOWN) {
            find_next = 1;
        } else if (c == ARROW_LEFT || c == ARROW_UP) {
            find_next = -1;
        } else if (isprint(c)) {
            if (qlen < EKILO_QUERY_LEN) {
                query[qlen++] = c;
                query[qlen] = '\0';
                last_match = -1;
            }
        }

        /* Search occurrence. */
        if (last_match == -1) find_next = 1;
        if (find_next) {
            char *match = NULL;
            int match_offset = 0;
            int i, current = last_match;

            for (i = 0; i < E.numrows; i++) {
                current += find_next;
                if (current == -1) current = E.numrows-1;
                else if (current == E.numrows) current = 0;
                match = strstr(E.row[current].render,query);
                if (match) {
                    match_offset = match-E.row[current].render;
                    break;
                }
            }
            find_next = 0;

            /* Highlight */
            FIND_RESTORE_HL;

            if (match) {
                erow *row = &E.row[current];
                last_match = current;
                if (row->hl) {
                    saved_hl_line = current;
                    saved_hl = malloc(row->rsize);
                    memcpy(saved_hl,row->hl,row->rsize);
                    memset(row->hl+match_offset,HL_MATCH,qlen);
                }
                E.cy = 0;
                E.cx = match_offset;
                E.rowoff = current;
                E.coloff = 0;
                /* Scroll horizontally as needed. */
                if (E.cx > E.screencols) {
                    int diff = E.cx - E.screencols;
                    E.cx -= diff;
                    E.coloff += diff;
                }
            }
        }
    }
}

/* ========================= Editor events handling  ======================== */

/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor(int key) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    int rowlen;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    switch(key) {
    case ARROW_LEFT:
        if (E.cx == 0) {
            if (E.coloff) {
                E.coloff--;
            } else {
                if (filerow > 0) {
                    E.cy--;
                    E.cx = E.row[filerow-1].size;
                    if (E.cx > E.screencols-1) {
                        E.coloff = E.cx-E.screencols+1;
                        E.cx = E.screencols-1;
                    }
                }
            }
        } else {
            E.cx -= 1;
        }
        break;
    case ARROW_RIGHT:
        if (row && filecol < row->size) {
            if (E.cx == E.screencols-1) {
                E.coloff++;
            } else {
                E.cx += 1;
            }
        } else if (row && filecol == row->size) {
            E.cx = 0;
            E.coloff = 0;
            if (E.cy == E.screenrows-1) {
                E.rowoff++;
            } else {
                E.cy += 1;
            }
        }
        break;
    case ARROW_UP:
        if (E.cy == 0) {
            if (E.rowoff) E.rowoff--;
        } else {
            E.cy -= 1;
        }
        break;
    case ARROW_DOWN:
        if (filerow < E.numrows) {
            if (E.cy == E.screenrows-1) {
                E.rowoff++;
            } else {
                E.cy += 1;
            }
        }
        break;
    }
    /* Fix cx if the current line has not enough chars. */
    filerow = E.rowoff+E.cy;
    filecol = E.coloff+E.cx;
    row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    rowlen = row ? row->size : 0;
    if (filecol > rowlen) {
        E.cx -= filecol-rowlen;
        if (E.cx < 0) {
            E.coloff += E.cx;
            E.cx = 0;
        }
    }
}

/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
#define EKILO_QUIT_TIMES 3
void editorProcessKeypress(int fd) {
    /* When the file is modified, requires Ctrl-q to be pressed N times
     * before actually quitting. */
    static int quit_times = EKILO_QUIT_TIMES;

    int c = editorReadKey(fd);
    E.last_key = c; // Store the last key pressed

    /* Paste mode detection */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (tv.tv_sec == E.last_char_time.tv_sec) {
        long elapsed = (tv.tv_usec - E.last_char_time.tv_usec);
        if (elapsed < 30000) E.paste_mode = 1; /* 30ms threshold */
    } else if (tv.tv_sec - E.last_char_time.tv_sec > 0) {
        E.paste_mode = 0;
    }
    E.last_char_time = tv;

    /* Original switch case */
    switch(c) {
    case ENTER:         /* Enter */
        editorInsertNewline();
        break;
    case CTRL_C:        /* Ctrl-c */
        /* We ignore ctrl-c, it can't be so simple to lose the changes
         * to the edited file. */
        break;
    case CTRL_Q:        /* Ctrl-q */
        /* Quit if the file was already saved. */
        if (E.dirty && quit_times) {
            editorSetStatusMessage("WARNING!!! File has unsaved changes. "
                "Press Ctrl-Q %d more times to quit.", quit_times);
            quit_times--;
            return;
        }

   	 /* Reset paste mode on non-char actions */
   	 if (c != ESC && !(c >= 32 && c <= 126)) {
    	    E.paste_mode = 0;
   	 }

        E.should_quit = 1;  // Set flag to quit gracefully instead of exit(0)
        break;
    case CTRL_S:        /* Ctrl-s */
        editorSave();
        break;
    case CTRL_F:
        editorFind(fd);
        break;
    case BACKSPACE:     /* Backspace */
    case CTRL_H:        /* Ctrl-h */
    case DEL_KEY:
        editorDelChar();
        break;
    case PAGE_UP:
    case PAGE_DOWN:
        if (c == PAGE_UP && E.cy != 0)
            E.cy = 0;
        else if (c == PAGE_DOWN && E.cy != E.screenrows-1)
            E.cy = E.screenrows-1;
        {
        int times = E.screenrows;
        while(times--)
            editorMoveCursor(c == PAGE_UP ? ARROW_UP:
                                            ARROW_DOWN);
        }
        break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        break;
    case CTRL_L: /* ctrl+l, clear screen */
        /* Just refresht the line as side effect. */
        break;
    case ESC:
        /* Nothing to do for ESC in this mode. */
        break;
    default:
        /* Replace editorInsertChar with our new autocomplete version */
        editorInsertCharAutoComplete(c);
        break;
    }

    quit_times = EKILO_QUIT_TIMES; /* Reset it to the original value. */
}

int editorFileWasModified(void) {
    return E.dirty;
}

void updateWindowSize(void) {
    int new_rows, new_cols;
    
    // Try to get window size, with retry logic
    int attempts = 0;
    const int max_attempts = 3;
    
    while (attempts < max_attempts) {
        if (getWindowSize(STDIN_FILENO, STDOUT_FILENO, &new_rows, &new_cols) == 0) {
            // Success - update the screen dimensions
            E.screenrows = new_rows - 2; /* Get room for status bar. */
            E.screencols = new_cols;
            return;
        }
        
        attempts++;
        if (attempts < max_attempts) {
            // Brief delay before retry (10ms)
            usleep(10000);
        }
    }
    
    // If all attempts failed, keep the current dimensions
    // and set a status message instead of exiting
    editorSetStatusMessage("Warning: Could not update window size");
}

void handleSigWinCh(int unused __attribute__((unused))) {
    updateWindowSize();
    
    // Ensure cursor position is within new screen bounds
    if (E.cy >= E.screenrows) E.cy = E.screenrows - 1;
    if (E.cx >= E.screencols) E.cx = E.screencols - 1;
    
    // Adjust offsets if necessary
    if (E.rowoff + E.cy >= E.numrows) {
        E.rowoff = E.numrows - E.cy - 1;
        if (E.rowoff < 0) E.rowoff = 0;
    }
    
    if (E.coloff + E.cx >= E.screencols) {
        E.coloff = 0;
        E.cx = E.screencols - 1;
    }
    
    editorRefreshScreen();
}

void initEditor(void) {
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.syntax = NULL;
    E.paste_mode = 0;
    E.last_char_time.tv_sec = 0;
    E.last_char_time.tv_usec = 0;
    E.should_quit = 0;
    updateWindowSize();
    signal(SIGWINCH, handleSigWinCh);
}

// Jumperless integration functions
// These functions allow eKilo to be called as a library from C++
int ekilo_main(const char* filename) {
    // If no filename provided, create a temporary new file
    char temp_filename[256];
    if (!filename || strlen(filename) == 0) {
        snprintf(temp_filename, sizeof(temp_filename), "untitled.txt");
        filename = temp_filename;
    }
    
    // Convert const char* to char* for compatibility
    char* mutable_filename = strdup(filename);
    if (!mutable_filename) {
        fprintf(stderr, "Memory allocation error\n");
        return 1;
    }
    
    // Initialize editor
    initEditor();
    editorSelectSyntaxHighlight(mutable_filename);
    
    // Try to open the file, create new if it doesn't exist
    if (editorOpen(mutable_filename) == -1) {
        // File doesn't exist, start with empty buffer
        E.filename = mutable_filename;
        E.dirty = 0;
    }
    
    // Enable raw mode for terminal input
    if (enableRawMode(STDIN_FILENO) == -1) {
        free(mutable_filename);
        return 1;
    }
    
    // Set help message
    editorSetStatusMessage(
        "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find | Ctrl-H = delete line");
    
    // Main editor loop
    while(!E.should_quit) {
        editorRefreshScreen();
        editorProcessKeypress(STDIN_FILENO);
    }
    
    // Restore terminal mode before exiting
    disableRawMode(STDIN_FILENO);
    
    // Clear screen and position cursor at bottom
    printf("\x1b[2J");
    printf("\x1b[H");
    printf("\x1b[%d;1H", E.screenrows + 2);  // Move cursor below the editor area
    
    // Cleanup
    free(mutable_filename);
    return 0;
}

void ekilo_init_for_jumperless() {
    // Any special initialization needed for Jumperless integration
    // Currently nothing special needed
}

// Check if editor has unsaved changes
int ekilo_has_unsaved_changes() {
    return E.dirty;
}

// Get current filename
const char* ekilo_get_filename() {
    return E.filename ? E.filename : "";
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr,"Usage: ekilo <filename>\n");
        exit(1);
    }

    initEditor();
    editorSelectSyntaxHighlight(argv[1]);
    editorOpen(argv[1]);
    enableRawMode(STDIN_FILENO);
    editorSetStatusMessage(
        "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find | Ctrl-H = delete line");
    while(1) {
        editorRefreshScreen();
        editorProcessKeypress(STDIN_FILENO);
    }
    return 0;
}
