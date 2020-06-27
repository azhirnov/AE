
struct CrashFileHeader
{
	static constexpr uint	VERSION = 1;
	static constexpr uint	MAGIC	= ('A') | ('E' << 8) | ('C' << 16) | ('R' << 24);

	uint		magic;
	uint		version;
	struct {
		uint		offset;		// offset in bytes from header end
		uint		size;
	}			symbolsId;		// wchar_t[]
	struct {
		uint		offset;		// offset in bytes from header end
		uint		size;
	}			userInfo;		// wchar_t[]
	struct {
		uint		offset;		// offset in bytes from header end
		uint		size;
	}			log;			// char[]
	struct {
		uint		offset;		// offset in bytes from header end
		uint		size;
	}			dump;			// uint8_t[]
};
