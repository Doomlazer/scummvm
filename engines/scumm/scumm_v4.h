/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SCUMM_SCRIPT_V4_H
#define SCUMM_SCRIPT_V4_H

#include "scumm/scumm_v5.h"

namespace Scumm {

/**
 * Engine for version 4 SCUMM games; GF_SMALL_HEADER is always set for these.
 */
class ScummEngine_v4 : public ScummEngine_v5 {
	friend class ScummEngine_v5;
public:

	/**
	 * Prepared savegame used by the original save/load dialog.
	 * Must be valid as long as the savescreen is active. As we are not
	 * notified when the savescreen is closed, memory is only freed on a game
	 * reset, at the destruction of the engine or when the original save/load
	 * dialog is entered the next time.
	 */
	Common::SeekableReadStream *_savePreparedSavegame;

	void prepareSavegame();
	bool savePreparedSavegame(int slot, char *desc);

public:
	ScummEngine_v4(OSystem *syst, const DetectorResult &dr);

	void resetScumm() override;

protected:
	const byte _GUIPalette[13]    = {0x00, 0x01, 0x0B, 0x03, 0x00, 0x0B, 0x0B, 0x03, 0x01, 0x00, 0x01, 0x0B, 0x09};
	const byte _GUIPaletteCGA[13] = {0x00, 0x03, 0x0B, 0x03, 0x00, 0x0B, 0x0B, 0x0F, 0x03, 0x00, 0x0B, 0x0B, 0x05};

	void setupOpcodes() override;

	int readResTypeList(ResType type) override;
	void readIndexFile() override;
	void loadCharset(int no) override;
	void resetRoomObjects() override;
	void readMAXS(int blockSize) override;
	void readGlobalObjects() override;

	void resetRoomObject(ObjectData *od, const byte *room, const byte *searchptr = NULL) override;

	void saveVars();
	void loadVars();
	void saveIQPoints();
	void loadIQPoints(byte *ptr, int size);
	void updateIQPoints();

	int getBannerColor(int bannerId) override;
	void setUpMainMenuControls() override;

	/* Version 4 script opcodes */
	void o4_ifState();
	void o4_ifNotState();
	void o4_oldRoomEffect();
	void o4_pickupObject();
	void o4_saveLoadGame();
	void o4_saveLoadVars();
};


} // End of namespace Scumm

#endif
