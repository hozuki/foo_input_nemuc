#pragma once

#include "../fb2k/foobar2000/SDK/filesystem.h"

class NOVTABLE mapped_file : public file
{

	public:

	virtual void set_source(file_ptr source) = 0;

	public:

	FB2K_MAKE_SERVICE_INTERFACE(mapped_file, file);

};

// File delegate; does XOR encryption/decryption during I/O
class NOVTABLE mapped_file_impl_t : public mapped_file
{

	// mapped_file
	public:

	void set_source(file_ptr source) override;

	// file
	public:

	t_filesize get_size(abort_callback& p_abort) override;

	t_filesize get_position(abort_callback& p_abort) override;

	void resize(t_filesize p_size, abort_callback& p_abort) override;

	void seek(t_filesize p_position, abort_callback& p_abort) override;

	void seek_ex(t_sfilesize p_position, file::t_seek_mode p_mode, abort_callback& p_abort) override;

	bool can_seek() override;

	bool get_content_type(pfc::string_base& p_out) override;

	bool is_in_memory() override;

	void on_idle(abort_callback& p_abort) override;

	t_filetimestamp get_timestamp(abort_callback& p_abort) override;

	void reopen(abort_callback& p_abort) override;

	bool is_remote() override;

	// stream_reader
	public:

	t_size read(void* p_buffer, t_size p_bytes, abort_callback& p_abort) override;

	void read_object(void* p_buffer, t_size p_bytes, abort_callback& p_abort) override;

	t_filesize skip(t_filesize p_bytes, abort_callback& p_abort) override;

	void skip_object(t_filesize p_bytes, abort_callback& p_abort) override;

	// stream_writer
	public:

	void write(const void* p_buffer, t_size p_bytes, abort_callback& p_abort) override;

	// impl members
	private:

	file_ptr m_source;

};
