/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Google Inc.
 *
 * mapped_framebuffer.cpp - Mapped Framebuffer support
 */

#include "libcamera/internal/mapped_framebuffer.h"

#include <errno.h>
#include <sys/mman.h>

#include <libcamera/base/log.h>

/**
 * \file libcamera/internal/mapped_framebuffer.h
 * \brief Frame buffer memory mapping support
 */

namespace libcamera {

LOG_DECLARE_CATEGORY(Buffer)

/**
 * \class MappedBuffer
 * \brief Provide an interface to support managing memory mapped buffers
 *
 * The MappedBuffer interface provides access to a set of MappedPlanes which
 * are available for access by the CPU.
 *
 * This class is not meant to be constructed directly, but instead derived
 * classes should be used to implement the correct mapping of a source buffer.
 *
 * This allows treating CPU accessible memory through a generic interface
 * regardless of whether it originates from a libcamera FrameBuffer or other
 * source.
 */

/**
 * \typedef MappedBuffer::Plane
 * \brief A mapped region of memory accessible to the CPU
 *
 * The MappedBuffer::Plane uses the Span interface to describe the mapped memory
 * region.
 */

/**
 * \brief Construct an empty MappedBuffer
 */
MappedBuffer::MappedBuffer()
	: error_(0)
{
}

/**
 * \brief Move constructor, construct the MappedBuffer with the contents of \a
 * other using move semantics
 * \param[in] other The other MappedBuffer
 *
 * Moving a MappedBuffer moves the mappings contained in the \a other to the new
 * MappedBuffer and invalidates the \a other.
 *
 * No mappings are unmapped or destroyed in this process.
 */
MappedBuffer::MappedBuffer(MappedBuffer &&other)
{
	*this = std::move(other);
}

/**
 * \brief Move assignment operator, replace the mappings with those of \a other
* \param[in] other The other MappedBuffer
 *
 * Moving a MappedBuffer moves the mappings contained in the \a other to the new
 * MappedBuffer and invalidates the \a other.
 *
 * No mappings are unmapped or destroyed in this process.
 */
MappedBuffer &MappedBuffer::operator=(MappedBuffer &&other)
{
	error_ = other.error_;
	maps_ = std::move(other.maps_);
	other.error_ = -ENOENT;

	return *this;
}

MappedBuffer::~MappedBuffer()
{
	for (Plane &map : maps_)
		munmap(map.data(), map.size());
}

/**
 * \fn MappedBuffer::isValid()
 * \brief Check if the MappedBuffer instance is valid
 * \return True if the MappedBuffer has valid mappings, false otherwise
 */

/**
 * \fn MappedBuffer::error()
 * \brief Retrieve the map error status
 *
 * This function retrieves the error status from the MappedBuffer.
 * The error status is a negative number as defined by errno.h. If
 * no error occurred, this function returns 0.
 *
 * \return The map error code
 */

/**
 * \fn MappedBuffer::maps()
 * \brief Retrieve the mapped planes
 *
 * This function retrieves the successfully mapped planes stored as a vector
 * of Span<uint8_t> to provide access to the mapped memory.
 *
 * \return A vector of the mapped planes
 */

/**
 * \var MappedBuffer::error_
 * \brief Stores the error value if present
 *
 * MappedBuffer derived classes shall set this to a negative value as defined
 * by errno.h if an error occured during the mapping process.
 */

/**
 * \var MappedBuffer::maps_
 * \brief Stores the internal mapped planes
 *
 * MappedBuffer derived classes shall store the mappings they create in this
 * vector which is parsed during destruct to unmap any memory mappings which
 * completed successfully.
 */

/**
 * \class MappedFrameBuffer
 * \brief Map a FrameBuffer using the MappedBuffer interface
 */

/**
 * \enum MappedFrameBuffer::MapFlag
 * \brief Specify the mapping mode for the FrameBuffer
 * \var MappedFrameBuffer::Read
 * \brief Create a read-only mapping
 * \var MappedFrameBuffer::Write
 * \brief Create a write-only mapping
 * \var MappedFrameBuffer::ReadWrite
 * \brief Create a mapping that can be both read and written
 */

/**
 * \typedef MappedFrameBuffer::MapFlags
 * \brief A bitwise combination of MappedFrameBuffer::MapFlag values
 */

/**
 * \brief Map all planes of a FrameBuffer
 * \param[in] buffer FrameBuffer to be mapped
 * \param[in] flags Protection flags to apply to map
 *
 * Construct an object to map a frame buffer for CPU access. The mapping can be
 * made as Read only, Write only or support Read and Write operations by setting
 * the MapFlag flags accordingly.
 */
MappedFrameBuffer::MappedFrameBuffer(const FrameBuffer *buffer, MapFlags flags)
{
	maps_.reserve(buffer->planes().size());

	int mmapFlags = 0;

	if (flags & MapFlag::Read)
		mmapFlags |= PROT_READ;

	if (flags & MapFlag::Write)
		mmapFlags |= PROT_WRITE;

	for (const FrameBuffer::Plane &plane : buffer->planes()) {
		void *address = mmap(nullptr, plane.length, mmapFlags,
				     MAP_SHARED, plane.fd.fd(), 0);
		if (address == MAP_FAILED) {
			error_ = -errno;
			LOG(Buffer, Error) << "Failed to mmap plane";
			break;
		}

		maps_.emplace_back(static_cast<uint8_t *>(address), plane.length);
	}
}

} /* namespace libcamera */
