// SoundPlayer.hh
//
// Copyright (C) 2002, 2003, 2006, 2007, 2008, 2009, 2010, 2011, 2013 Rob Caelers & Raymond Penners
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef SOUNDPLAYER_HH
#define SOUNDPLAYER_HH

#include <string>
#include <list>
#include <vector>
#include <map>

#include "config/IConfigurator.hh"

#include "ISoundDriver.hh"

class IMixer;

class SoundPlayer : public workrave::audio::ISoundPlayer, public ISoundPlayerEvents
{
public:
  SoundPlayer();
  virtual ~SoundPlayer();

  void play_sound(workrave::audio::SoundEvent snd, const std::string &wavfile, bool mute_after_playback, int volume);
  void play_sound(const std::string &wavfile, int volume);

  void init();
  bool capability(workrave::audio::SoundCapability cap);
  void restore_mute();

  void eos_event();

private:
  ISoundDriver *driver;
  IMixer *mixer;
  bool delayed_mute;
  bool must_unmute;
};

#endif // SOUNDPLAYER_HH