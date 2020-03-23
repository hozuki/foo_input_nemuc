#include "stdafx.h"
#include "mapped_file.h"

static const std::uint8_t XorKey = 0xa3;

const GUID mapped_file_impl_t::class_guid = { 0xfdd68e4a, 0xdd9c, 0x405e, {0x8b, 0xc0, 0x19, 0xfb, 0xa3, 0x90, 0x04,0xcd} };

void mapped_file_impl_t::set_source(file_ptr source)
{
	this->m_source = source;
}

t_filesize mapped_file_impl_t::get_size(abort_callback& p_abort)
{
	return m_source->get_size(p_abort);
}

t_filesize mapped_file_impl_t::get_position(abort_callback& p_abort)
{
	return m_source->get_position(p_abort);
}

void mapped_file_impl_t::resize(t_filesize p_size, abort_callback& p_abort)
{
	m_source->resize(p_size, p_abort);
}

void mapped_file_impl_t::seek(t_filesize p_position, abort_callback& p_abort)
{
	m_source->seek(p_position, p_abort);
}

void mapped_file_impl_t::seek_ex(t_sfilesize p_position, file::t_seek_mode p_mode, abort_callback& p_abort)
{
	m_source->seek_ex(p_position, p_mode, p_abort);
}

bool mapped_file_impl_t::can_seek()
{
	return m_source->can_seek();
}

bool mapped_file_impl_t::get_content_type(pfc::string_base& p_out)
{
	return m_source->get_content_type(p_out);
}

bool mapped_file_impl_t::is_in_memory()
{
	return m_source->is_in_memory();
}

void mapped_file_impl_t::on_idle(abort_callback& p_abort)
{
	m_source->on_idle(p_abort);
}

t_filetimestamp mapped_file_impl_t::get_timestamp(abort_callback& p_abort)
{
	return m_source->get_timestamp(p_abort);
}

void mapped_file_impl_t::reopen(abort_callback& p_abort)
{
	m_source->reopen(p_abort);
}

bool mapped_file_impl_t::is_remote()
{
	return m_source->is_remote();
}

t_size mapped_file_impl_t::read(void* p_buffer, t_size p_bytes, abort_callback& p_abort)
{
	pfc::array_t<uint8_t> buf;

	buf.resize(p_bytes);

	const auto read = m_source->read(buf.get_ptr(), p_bytes, p_abort);

	if (p_abort.is_aborting())
	{
		return read;
	}

	if (read > 0)
	{
		for (size_t i = 0; i < read; i += 1)
		{
			buf[i] = buf[i] ^ XorKey;
		}

		memcpy(p_buffer, buf.get_ptr(), read);
	}

	return read;
}

void mapped_file_impl_t::read_object(void* p_buffer, t_size p_bytes, abort_callback& p_abort)
{
	m_source->read_object(p_buffer, p_bytes, p_abort);
}

t_filesize mapped_file_impl_t::skip(t_filesize p_bytes, abort_callback& p_abort)
{
	return m_source->skip(p_bytes, p_abort);
}

void mapped_file_impl_t::skip_object(t_filesize p_bytes, abort_callback& p_abort)
{
	m_source->skip_object(p_bytes, p_abort);
}

void mapped_file_impl_t::write(const void* p_buffer, t_size p_bytes, abort_callback& p_abort)
{
	pfc::array_t<uint8_t> buf;

	buf.resize(p_bytes);

	if (p_bytes > 0)
	{
		memcpy(buf.get_ptr(), p_buffer, p_bytes);

		for (size_t i = 0; i < p_bytes; i += 1)
		{
			buf[i] = buf[i] ^ XorKey;
		}
	}

	m_source->write(buf.get_ptr(), p_bytes, p_abort);
}
