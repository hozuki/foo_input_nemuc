#include "stdafx.h"
#include "mapped_file.h"

// Note that input class does *not* implement virtual methods or derive from interface classes.
// Our methods get called over input framework templates. See input_singletrack_impl for descriptions of what each method does.
// input_stubs just provides stub implementations of mundane methods that are irrelevant for most implementations.
class input_nemuc : public input_stubs
{
	public:

	void open(service_ptr_t<file> p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort)
	{
		//p_filehint may be null, hence next line
		m_original_file = p_filehint;

		//if m_original_file is null, opens file with appropriate privileges for our operation (read/write for writing tags, read-only otherwise).
		input_open_file_helper(m_original_file, p_path, p_reason, p_abort);

		m_mapped_file = fb2k::service_new<mapped_file_impl_t>();

		m_mapped_file->set_source(m_original_file);

		const auto path = std::string(p_path);

		// Replaces ".uc" extension with ".mp3"
		m_fake_path = path.substr(0, path.length() - 3) + ".mp3";
	}

	void get_info(file_info& p_info, abort_callback& p_abort)
	{
		if (!try_open_audio_worker(input_open_info_read, p_abort))
		{
			return;
		}

		m_audio_info_reader->get_info(0, p_info, p_abort);
	}

	t_filestats get_file_stats(abort_callback& p_abort)
	{
		if (!try_open_audio_worker(input_open_info_read, p_abort))
		{
			return filestats_invalid;
		}

		return m_audio_info_reader->get_file_stats(p_abort);
	}

	void decode_initialize(unsigned p_flags, abort_callback& p_abort)
	{
		if (!try_open_audio_worker(input_open_decode, p_abort))
		{
			return;
		}

		m_audio_decoder->initialize(0, p_flags, p_abort);
	}

	bool decode_run(audio_chunk& p_chunk, abort_callback& p_abort)
	{
		if (!try_open_audio_worker(input_open_decode, p_abort))
		{
			return false;
		}

		return m_audio_decoder->run(p_chunk, p_abort);
	}

	void decode_seek(double p_seconds, abort_callback& p_abort)
	{
		if (!try_open_audio_worker(input_open_decode, p_abort))
		{
			return;
		}

		m_audio_decoder->seek(p_seconds, p_abort);
	}

	bool decode_can_seek()
	{
		abort_callback_dummy noAbort;

		if (!try_open_audio_worker(input_open_decode, noAbort))
		{
			return false;
		}

		return m_audio_decoder->can_seek();
	}

	// deals with dynamic information such as VBR bitrates
	bool decode_get_dynamic_info(file_info& p_out, double& p_timestamp_delta)
	{
		abort_callback_dummy noAbort;

		if (!try_open_audio_worker(input_open_decode, noAbort))
		{
			return false;
		}

		return m_audio_decoder->get_dynamic_info(p_out, p_timestamp_delta);
	}

	// deals with dynamic information such as track changes in live streams
	bool decode_get_dynamic_info_track(file_info& p_out, double& p_timestamp_delta)
	{
		abort_callback_dummy noAbort;

		if (!try_open_audio_worker(input_open_decode, noAbort))
		{
			return false;
		}

		return m_audio_decoder->get_dynamic_info_track(p_out, p_timestamp_delta);
	}

	void decode_on_idle(abort_callback& p_abort)
	{
		if (!try_open_audio_worker(input_open_decode, p_abort))
		{
			return;
		}

		m_audio_decoder->on_idle(p_abort);
	}

	void retag(const file_info& p_info, abort_callback& p_abort)
	{
		if (!try_open_audio_worker(input_open_info_write, p_abort))
		{
			return;
		}

		m_audio_info_writer->set_info(0, p_info, p_abort);
	}

	// match against supported mime types here
	static bool g_is_our_content_type(const char* p_content_type)
	{
		return false;
	}

	static bool g_is_our_path(const char* p_path, const char* p_extension)
	{
		// TODO: Also check existence of companion .info file
		return stricmp_utf8(p_extension, "uc") == 0;
	}

	static const char* g_get_name()
	{
		return "NetEase Music Encrypted";
	}

	static const GUID g_get_guid()
	{
		static const GUID guid = { 0x65c83841, 0x119c, 0x4066,{ 0xa0, 0x26, 0xe7, 0x10, 0xe2, 0x4c, 0x0d, 0x4a } };
		return guid;
	}

	private:

	bool try_get_audio_format(const char* path)
	{
		if (m_audio_format.is_valid())
		{
			// We already recognized its format
			return true;
		}

		if (path == nullptr || path[0] == '\0')
		{
			return false;
		}

		pfc::list_t<service_ptr_t<input_entry>> result;

		// TODO: Should parse companion .info (a JSON file) to assert actual file type.
		// Its content is like:
		//   {"format":"mp3","volume":0.0}
		// By now most of those cached files are MP3s, but there may be FLACs or other formats. (to be verified)

		// "audio/mpeg" is the MIME type for MP3 audio
		auto found = input_entry::g_find_inputs_by_content_type(result, "audio/mpeg", false);

		if (!found)
		{
			// If we failed to determine input independently, try with file path (beware, it's already modified by us)
			found = input_entry::g_find_inputs_by_path(result, path, false);
		}

		if (found)
		{
			// Use the first recognized format
			m_audio_format = result[0];
		}

		return found;
	}

	bool try_open_audio_worker(t_input_open_reason reason, abort_callback& abort)
	{
		const auto path = m_fake_path.empty() ? nullptr : m_fake_path.c_str();
		return try_open_audio_worker(path, reason, abort);
	}

	bool try_open_audio_worker(const char* path, t_input_open_reason reason, abort_callback& abort)
	{
		if (!try_get_audio_format(path))
		{
			return false;
		}

		switch (reason)
		{
		case input_open_decode:
		{
			if (m_audio_decoder.is_valid())
			{
				return true;
			}
			m_audio_format->open_for_decoding(m_audio_decoder, m_mapped_file, path, abort);
			return m_audio_decoder.is_valid();
		}
		case input_open_info_read:
		{
			if (m_audio_info_reader.is_valid())
			{
				return true;
			}
			m_audio_format->open_for_info_read(m_audio_info_reader, m_mapped_file, path, abort);
			return m_audio_info_reader.is_valid();
		}
		case input_open_info_write:
		{
			if (m_audio_info_writer.is_valid())
			{
				return true;
			}
			m_audio_format->open_for_info_write(m_audio_info_writer, m_mapped_file, path, abort);
			return m_audio_info_writer.is_valid();
		}
		default:
			break;
		}

		return false;
	}

	private:

	service_ptr_t<file> m_original_file;
	service_ptr_t<mapped_file> m_mapped_file;
	std::string m_fake_path;

	private:

	service_ptr_t<input_entry> m_audio_format;
	service_ptr_t<input_decoder> m_audio_decoder;
	service_ptr_t<input_info_reader> m_audio_info_reader;
	service_ptr_t<input_info_writer> m_audio_info_writer;

};

static input_singletrack_factory_t<input_nemuc> g_input_nemuc_factory;

// Declare .RAW as a supported file type to make it show in "open file" dialog etc.
// ^ Nah. It's not there.
DECLARE_FILE_TYPE("NetEase Music Encrypted", "*.uc");
