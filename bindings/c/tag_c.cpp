/***************************************************************************
    copyright            : (C) 2003 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <fileref.h>
#include <tfile.h>
#include <asffile.h>
#include <vorbisfile.h>
#include <mpegfile.h>
#include <flacfile.h>
#include <oggflacfile.h>
#include <mpcfile.h>
#include <wavpackfile.h>
#include <speexfile.h>
#include <trueaudiofile.h>
#include <mp4file.h>
#include <tag.h>
#include <string.h>
#include <id3v2framefactory.h>

#include <sstream>
#include <tpropertymap.h>

#include <id3v2tag.h>
#include <id3v2frame.h> //frame
#include <attachedPictureFrame.h>

#include "tag_c.h"

using namespace TagLib;

String _get_JSON_string(TagLib::PropertyMap p);

namespace
{
  List<char *> strings;
  bool unicodeStrings = true;
  bool stringManagementEnabled = true;

  char *stringToCharArray(const String &s)
  {
    const std::string str = s.to8Bit(unicodeStrings);

#ifdef HAVE_ISO_STRDUP

    return ::_strdup(str.c_str());

#else

    return ::strdup(str.c_str());

#endif
  }

  String charArrayToString(const char *s)
  {
    return String(s, unicodeStrings ? String::UTF8 : String::Latin1);
  }
}

void taglib_set_strings_unicode(BOOL unicode)
{
  unicodeStrings = (unicode != 0);
}

void taglib_set_string_management_enabled(BOOL management)
{
  stringManagementEnabled = (management != 0);
}

void taglib_free(void* pointer)
{
  free(pointer);
}

////////////////////////////////////////////////////////////////////////////////
// TagLib::File wrapper
////////////////////////////////////////////////////////////////////////////////

TagLib_File *taglib_file_new(const char *filename)
{
  return reinterpret_cast<TagLib_File *>(FileRef::create(filename));
}

TagLib_File *taglib_file_new_type(const char *filename, TagLib_File_Type type)
{
  switch(type) {
  case TagLib_File_MPEG:
    return reinterpret_cast<TagLib_File *>(new MPEG::File(filename));
  case TagLib_File_OggVorbis:
    return reinterpret_cast<TagLib_File *>(new Ogg::Vorbis::File(filename));
  case TagLib_File_FLAC:
    return reinterpret_cast<TagLib_File *>(new FLAC::File(filename));
  case TagLib_File_MPC:
    return reinterpret_cast<TagLib_File *>(new MPC::File(filename));
  case TagLib_File_OggFlac:
    return reinterpret_cast<TagLib_File *>(new Ogg::FLAC::File(filename));
  case TagLib_File_WavPack:
    return reinterpret_cast<TagLib_File *>(new WavPack::File(filename));
  case TagLib_File_Speex:
    return reinterpret_cast<TagLib_File *>(new Ogg::Speex::File(filename));
  case TagLib_File_TrueAudio:
    return reinterpret_cast<TagLib_File *>(new TrueAudio::File(filename));
  case TagLib_File_MP4:
    return reinterpret_cast<TagLib_File *>(new MP4::File(filename));
  case TagLib_File_ASF:
    return reinterpret_cast<TagLib_File *>(new ASF::File(filename));
  default:
    return 0;
  }
}

void taglib_file_free(TagLib_File *file)
{
  delete reinterpret_cast<File *>(file);
}

BOOL taglib_file_is_valid(const TagLib_File *file)
{
  return reinterpret_cast<const File *>(file)->isValid();
}

TagLib_Tag *taglib_file_tag(const TagLib_File *file)
{
  const File *f = reinterpret_cast<const File *>(file);
  return reinterpret_cast<TagLib_Tag *>(f->tag());
}

const TagLib_AudioProperties *taglib_file_audioproperties(const TagLib_File *file)
{
  const File *f = reinterpret_cast<const File *>(file);
  return reinterpret_cast<const TagLib_AudioProperties *>(f->audioProperties());
}

BOOL taglib_file_save(TagLib_File *file)
{
  return reinterpret_cast<File *>(file)->save();
}

BOOL taglib_file_property_attrs(const TagLib_File *file, const char *key, unsigned int *count, size_t *length)
{
	const File *f = reinterpret_cast<const File *>(file);
	TagLib::PropertyMap p = f->properties();
	PropertyMap::Iterator i = p.find(key);
	if (i != p.end()) {
		*count = i->second.size();
		std::stringstream ss;
		for (TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
			if (j != i->second.begin()) {
				ss << "; ";
			}
			ss << *j;
		}
		*length = ss.str().length();
		return true;
	}
	return false;
}

char *taglib_file_property(const TagLib_File *file, const char *key)
{
	const File *f = reinterpret_cast<const File *>(file);
	TagLib::PropertyMap p = f->properties();
	PropertyMap::Iterator i = p.find(key);
	if (i != p.end()) {
		std::stringstream ss;
		for (TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
			if (j != i->second.begin()) {
				ss << "; ";
			}
			ss << *j;
		}
		char *s = stringToCharArray(ss.str());
		if (stringManagementEnabled)
			strings.append(s);
		return s;
	}
	return "";
}

char *taglib_file_property_map_to_JSON(const TagLib_File *file)
{
	const File *f = reinterpret_cast<const File *>(file);
	TagLib::PropertyMap p = f->properties();
	char *s = stringToCharArray(_get_JSON_string(p));
	if (stringManagementEnabled)
		strings.append(s);
	return s;
}

size_t taglib_file_property_map_to_JSON_length(const TagLib_File *file)
{
	const File *f = reinterpret_cast<const File *>(file);
	TagLib::PropertyMap p = f->properties();
	String s = _get_JSON_string(p);
	return s.length();
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

String _get_JSON_string(TagLib::PropertyMap p)
{
	PropertyMap::Iterator i = p.begin();
	std::stringstream ss;
	ss << "{ ";
	while (i != p.end()) {
		if (i != p.begin()) {
			ss << ", ";
		}
		ss << "\"" << i->first << "\": \"";
		for (TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
			if (j != i->second.begin()) {
				ss << "; ";
			}
			ss << ReplaceAll(j->toCString(), "\"", "\\\"");		// escape double quotes
		}
		ss << "\"";
		i++;
	}
	ss << " }";
	return ss.str();
}

char *taglib_file_property_key_index(const TagLib_File *file, const unsigned int index)
{
	const File *f = reinterpret_cast<const File *>(file);
	TagLib::PropertyMap p = f->properties();
	if (index < p.size()) {
		PropertyMap::Iterator i = p.begin();
		unsigned int j = 0;
		while (j < index && i != p.end()) {
			++i;
			++j;
		}
		if (i != p.end()) {
			char *s = stringToCharArray(i->first);
			if (stringManagementEnabled)
				strings.append(s);
			return s;
		}
	}
	return "";
}

char *taglib_mp3_file_picture_attrs(TagLib_File *file, unsigned int *type)
{
	MPEG::File *f = reinterpret_cast<MPEG::File *>(file);
	ID3v2::Tag *mp3Tag = f->ID3v2Tag();
	ID3v2::FrameList frameList = mp3Tag->frameList("APIC");
	ID3v2::AttachedPictureFrame *pictureFrame;
	if (!frameList.isEmpty()) {
		ID3v2::FrameList::ConstIterator it = frameList.begin();
		pictureFrame = static_cast<ID3v2::AttachedPictureFrame *> (*it);
		char *s = stringToCharArray(pictureFrame->mimeType());
		if (stringManagementEnabled)
			strings.append(s);
		*type = pictureFrame->type();

		return s;
	}
	return "";
}

BOOL taglib_mp3_file_picture(TagLib_File *file, const char *filename)
{
	MPEG::File *f = reinterpret_cast<MPEG::File *>(file);
	ID3v2::Tag *mp3Tag = f->ID3v2Tag();
	ID3v2::FrameList frameList = mp3Tag->frameList("APIC");
	ID3v2::AttachedPictureFrame *pictureFrame;
	if (!frameList.isEmpty()) {
		ID3v2::FrameList::ConstIterator it = frameList.begin();
		pictureFrame = static_cast<ID3v2::AttachedPictureFrame *> (*it);
		
		FILE * fout;
		fopen_s(&fout, filename, "wb");
		fwrite(pictureFrame->picture().data(), pictureFrame->picture().size(), 1, fout);
		fclose(fout);
		return true;
	}
	return false;
}

void taglib_mp3_file_remove_picture(TagLib_File *file)
{
	MPEG::File *f = reinterpret_cast<MPEG::File *>(file);
	ID3v2::Tag *mp3Tag = f->ID3v2Tag();
	ID3v2::FrameList frameList = mp3Tag->frameList("APIC");
	if (!frameList.isEmpty()) {
		ID3v2::Frame *frame = (*frameList.begin());
		mp3Tag->removeFrame(frame);
	}
}

void taglib_file_set_property(TagLib_File *file, const char *key, const char *value, BOOL multiValue)
{
	File *f = reinterpret_cast<File *>(file);
	TagLib::PropertyMap p = f->properties();
	TagLib::StringList valList;
	if (multiValue) {
		valList = TagLib::String(value).split("; ");
	} else {
		valList.append(TagLib::String(value));
	}
	p.erase(TagLib::String(key));

	TagLib::StringList::Iterator it = valList.begin();
	while (it != valList.end()) {
		if (it->isEmpty()) {
			valList.erase(it++);
		} else {
			it++;
		}
	}
	if (valList.size()) {
		p.insert(TagLib::String(key), valList);
	}
	f->setProperties(p);
}

////////////////////////////////////////////////////////////////////////////////
// TagLib::Tag wrapper
////////////////////////////////////////////////////////////////////////////////

char *taglib_tag_title(const TagLib_Tag *tag)
{
  const Tag *t = reinterpret_cast<const Tag *>(tag);
  char *s = stringToCharArray(t->title());
  if(stringManagementEnabled)
    strings.append(s);
  return s;
}

char *taglib_tag_artist(const TagLib_Tag *tag)
{
  const Tag *t = reinterpret_cast<const Tag *>(tag);
  char *s = stringToCharArray(t->artist());
  if(stringManagementEnabled)
    strings.append(s);
  return s;
}

char *taglib_tag_album(const TagLib_Tag *tag)
{
  const Tag *t = reinterpret_cast<const Tag *>(tag);
  char *s = stringToCharArray(t->album());
  if(stringManagementEnabled)
    strings.append(s);
  return s;
}

char *taglib_tag_comment(const TagLib_Tag *tag)
{
  const Tag *t = reinterpret_cast<const Tag *>(tag);
  char *s = stringToCharArray(t->comment());
  if(stringManagementEnabled)
    strings.append(s);
  return s;
}

char *taglib_tag_genre(const TagLib_Tag *tag)
{
  const Tag *t = reinterpret_cast<const Tag *>(tag);
  char *s = stringToCharArray(t->genre());
  if(stringManagementEnabled)
    strings.append(s);
  return s;
}

unsigned int taglib_tag_year(const TagLib_Tag *tag)
{
  const Tag *t = reinterpret_cast<const Tag *>(tag);
  return t->year();
}

unsigned int taglib_tag_track(const TagLib_Tag *tag)
{
  const Tag *t = reinterpret_cast<const Tag *>(tag);
  return t->track();
}

void taglib_tag_set_title(TagLib_Tag *tag, const char *title)
{
  Tag *t = reinterpret_cast<Tag *>(tag);
  t->setTitle(charArrayToString(title));
}

void taglib_tag_set_artist(TagLib_Tag *tag, const char *artist)
{
  Tag *t = reinterpret_cast<Tag *>(tag);
  t->setArtist(charArrayToString(artist));
}

void taglib_tag_set_album(TagLib_Tag *tag, const char *album)
{
  Tag *t = reinterpret_cast<Tag *>(tag);
  t->setAlbum(charArrayToString(album));
}

void taglib_tag_set_comment(TagLib_Tag *tag, const char *comment)
{
  Tag *t = reinterpret_cast<Tag *>(tag);
  t->setComment(charArrayToString(comment));
}

void taglib_tag_set_genre(TagLib_Tag *tag, const char *genre)
{
  Tag *t = reinterpret_cast<Tag *>(tag);
  t->setGenre(charArrayToString(genre));
}

void taglib_tag_set_year(TagLib_Tag *tag, unsigned int year)
{
  Tag *t = reinterpret_cast<Tag *>(tag);
  t->setYear(year);
}

void taglib_tag_set_track(TagLib_Tag *tag, unsigned int track)
{
  Tag *t = reinterpret_cast<Tag *>(tag);
  t->setTrack(track);
}

void taglib_tag_free_strings()
{
  if(!stringManagementEnabled)
    return;

  for(List<char *>::ConstIterator it = strings.begin(); it != strings.end(); ++it)
    free(*it);
  strings.clear();
}

////////////////////////////////////////////////////////////////////////////////
// TagLib::AudioProperties wrapper
////////////////////////////////////////////////////////////////////////////////

int taglib_audioproperties_length(const TagLib_AudioProperties *audioProperties)
{
  const AudioProperties *p = reinterpret_cast<const AudioProperties *>(audioProperties);
  return p->length();
}

int taglib_audioproperties_bitrate(const TagLib_AudioProperties *audioProperties)
{
  const AudioProperties *p = reinterpret_cast<const AudioProperties *>(audioProperties);
  return p->bitrate();
}

int taglib_audioproperties_samplerate(const TagLib_AudioProperties *audioProperties)
{
  const AudioProperties *p = reinterpret_cast<const AudioProperties *>(audioProperties);
  return p->sampleRate();
}

int taglib_audioproperties_channels(const TagLib_AudioProperties *audioProperties)
{
  const AudioProperties *p = reinterpret_cast<const AudioProperties *>(audioProperties);
  return p->channels();
}

void taglib_id3v2_set_default_text_encoding(TagLib_ID3v2_Encoding encoding)
{
  String::Type type = String::Latin1;

  switch(encoding)
  {
  case TagLib_ID3v2_Latin1:
    type = String::Latin1;
    break;
  case TagLib_ID3v2_UTF16:
    type = String::UTF16;
    break;
  case TagLib_ID3v2_UTF16BE:
    type = String::UTF16BE;
    break;
  case TagLib_ID3v2_UTF8:
    type = String::UTF8;
    break;
  }

  ID3v2::FrameFactory::instance()->setDefaultTextEncoding(type);
}
