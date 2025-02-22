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

#include "kyra/gui/gui_v2.h"
#include "kyra/graphics/screen_v2.h"
#include "kyra/text/text.h"
#include "kyra/engine/util.h"

#include "common/savefile.h"
#include "common/system.h"

namespace Kyra {

GUI_v2::GUI_v2(KyraEngine_v2 *vm) : GUI_v1(vm), _vm(vm), _screen(vm->screen_v2()) {
	_backUpButtonList = _specialProcessButton = nullptr;
	_buttonListChanged = false;
	_flagsModifier = 0;

	_currentMenu = nullptr;
	_isDeathMenu = false;
	_isSaveMenu = false;
	_isLoadMenu = false;
	_scrollUpFunctor = BUTTON_FUNCTOR(GUI_v2, this, &GUI_v2::scrollUpButton);
	_scrollDownFunctor = BUTTON_FUNCTOR(GUI_v2, this, &GUI_v2::scrollDownButton);
	_sliderHandlerFunctor = BUTTON_FUNCTOR(GUI_v2, this, &GUI_v2::sliderHandler);
	_savegameOffset = 0;
	_isDeleteMenu = false;
	_saveMenuFont = Screen::FID_8_FNT;
	_saveMenuCursor = Common::Rect(1, 1, 7, 8);
	_saveLoadNumSlots = 5;
	_isChoiceMenu = _isOptionsMenu = _madeSave = _loadedSave = _restartGame = _reloadTemporarySave = false;
	_noLoadProcess = _noSaveProcess = _choice = _finishNameInput = _cancelNameInput = false;
	_saveSlot = _slotToDelete = 0;

	if (vm->game() == GI_KYRA2 && vm->gameFlags().lang == Common::ZH_TWN) {
		_saveMenuFont = Screen::FID_CHINESE_FNT;
		_saveMenuCursor = Common::Rect(0, 0, 8, 14);
		_saveLoadNumSlots = 4;
	}
}

Button *GUI_v2::addButtonToList(Button *list, Button *newButton) {
	list = GUI_v1::addButtonToList(list, newButton);
	_buttonListChanged = true;
	return list;
}

void GUI_v2::processButton(Button *button) {
	if (!button)
		return;

	if (button->flags & 8) {
		if (button->flags & 0x10) {
			// XXX
		}
		return;
	}

	int entry = button->flags2 & 5;

	byte val1 = 0, val2 = 0, val3 = 0;
	const uint8 *dataPtr = nullptr;
	Button::Callback callback;
	if (entry == 1) {
		val1 = button->data1Val1;
		dataPtr = button->data1ShapePtr;
		callback = button->data1Callback;
		val2 = button->data1Val2;
		val3 = button->data1Val3;
	} else if (entry == 4 || entry == 5) {
		val1 = button->data2Val1;
		dataPtr = button->data2ShapePtr;
		callback = button->data2Callback;
		val2 = button->data2Val2;
		val3 = button->data2Val3;
	} else {
		val1 = button->data0Val1;
		dataPtr = button->data0ShapePtr;
		callback = button->data0Callback;
		val2 = button->data0Val2;
		val3 = button->data0Val3;
	}

	int x = 0, y = 0, x2 = 0, y2 = 0;

	x = button->x;
	if (x < 0)
		x += _screen->getScreenDim(button->dimTableIndex)->w << 3;
	x += _screen->getScreenDim(button->dimTableIndex)->sx << 3;
	x2 = x + button->width - 1;

	y = button->y;
	if (y < 0)
		y += _screen->getScreenDim(button->dimTableIndex)->h << 3;
	y += _screen->getScreenDim(button->dimTableIndex)->sy << 3;
	y2 = y + button->height - 1;

	switch (val1 - 1) {
	case 0:
		_screen->drawShape(_screen->_curPage, dataPtr, x, y, button->dimTableIndex, 0x10);
		break;

	case 1:
		_screen->printText((const char *)dataPtr, x, y, val2, val3);
		break;

	case 3:
		if (callback)
			(*callback)(button);
		break;

	case 4:
		_screen->drawBox(x, y, x2, y2, val2);
		break;

	case 5:
		_screen->fillRect(x, y, x2, y2, val2, -1, true);
		break;

	default:
		break;
	}
}

int GUI_v2::processButtonList(Button *buttonList, uint16 inputFlag, int8 mouseWheel) {
	if (!buttonList)
		return inputFlag & 0x7FFF;

	if (_backUpButtonList != buttonList || _buttonListChanged) {
		_specialProcessButton = nullptr;
		//flagsModifier |= 0x2200;
		_backUpButtonList = buttonList;
		_buttonListChanged = false;

		while (buttonList) {
			processButton(buttonList);
			buttonList = buttonList->nextButton;
		}
	}

	Common::Point p = _vm->getMousePos();
	int mouseX = _vm->_mouseX = p.x;
	int mouseY = _vm->_mouseY = p.y;

	uint16 flags = 0;

	if (1/*!_screen_cursorDisable*/) {
		uint16 inFlags = inputFlag & 0xFF;
		uint16 temp = 0;

		// HACK: inFlags == 200 is our left button (up)
		if (inFlags == 199 || inFlags == 200)
			temp = 0x1000;
		if (inFlags == 198)
			temp = 0x100;

		if (inputFlag & 0x800)
			temp <<= 2;

		flags |= temp;

		_flagsModifier &= ~((temp & 0x4400) >> 1);
		_flagsModifier |= (temp & 0x1100) * 2;
		flags |= _flagsModifier;
		flags |= (_flagsModifier << 2) ^ 0x8800;
	}

	buttonList = _backUpButtonList;
	if (_specialProcessButton) {
		buttonList = _specialProcessButton;
		if (_specialProcessButton->flags & 8)
			_specialProcessButton = nullptr;
	}

	int returnValue = 0;
	while (buttonList) {
		if (buttonList->flags & 8) {
			buttonList = buttonList->nextButton;
			continue;
		}
		buttonList->flags2 &= ~0x18;
		buttonList->flags2 |= (buttonList->flags2 & 3) << 3;

		int x = buttonList->x;
		if (x < 0)
			x += _screen->getScreenDim(buttonList->dimTableIndex)->w << 3;
		x += _screen->getScreenDim(buttonList->dimTableIndex)->sx << 3;

		int y = buttonList->y;
		if (y < 0)
			y += _screen->getScreenDim(buttonList->dimTableIndex)->h;
		y += _screen->getScreenDim(buttonList->dimTableIndex)->sy;

		bool progress = false;

		if (mouseX >= x && mouseY >= y && mouseX <= x + buttonList->width && mouseY <= y + buttonList->height)
			progress = true;

		buttonList->flags2 &= ~0x80;
		uint16 inFlags = inputFlag & 0x7FFF;
		if (inFlags) {
			if (buttonList->keyCode == inFlags) {
				progress = true;
				flags = buttonList->flags & 0x0F00;
				buttonList->flags2 |= 0x80;
				inputFlag = 0;
				_specialProcessButton = buttonList;
			} else if (buttonList->keyCode2 == inFlags) {
				flags = buttonList->flags & 0xF000;
				if (!flags)
					flags = buttonList->flags & 0x0F00;
				progress = true;
				buttonList->flags2 |= 0x80;
				inputFlag = 0;
				_specialProcessButton = buttonList;
			}
		}

		bool unk1 = false;

		if (mouseWheel && buttonList->mouseWheel == mouseWheel) {
			progress = true;
			unk1 = true;
		}

		if (!progress)
			buttonList->flags2 &= ~6;

		if ((flags & 0x3300) && (buttonList->flags & 4) && progress && (buttonList == _specialProcessButton || !_specialProcessButton)) {
			buttonList->flags |= 6;
			if (!_specialProcessButton)
				_specialProcessButton = buttonList;
		} else if ((flags & 0x8800) && !(buttonList->flags & 4) && progress) {
			buttonList->flags2 |= 6;
		} else {
			buttonList->flags2 &= ~6;
		}

		bool progressSwitch = false;
		if (!_specialProcessButton) {
			progressSwitch = progress;
		} else  {
			if (_specialProcessButton->flags & 0x40)
				progressSwitch = (_specialProcessButton == buttonList);
			else
				progressSwitch = progress;
		}

		if (progressSwitch) {
			if ((flags & 0x1100) && progress && !_specialProcessButton) {
				inputFlag = 0;
				_specialProcessButton = buttonList;
			}

			if ((buttonList->flags & flags) && (progress || !(buttonList->flags & 1))) {
				uint16 combinedFlags = (buttonList->flags & flags);
				combinedFlags = ((combinedFlags & 0xF000) >> 4) | (combinedFlags & 0x0F00);
				combinedFlags >>= 8;

				static const uint16 flagTable[] = {
					0x000, 0x100, 0x200, 0x100, 0x400, 0x100, 0x400, 0x100, 0x800, 0x100,
					0x200, 0x100, 0x400, 0x100, 0x400, 0x100
				};

				assert(combinedFlags < ARRAYSIZE(flagTable));

				switch (flagTable[combinedFlags]) {
				case 0x400:
					if (!(buttonList->flags & 1) || ((buttonList->flags & 1) && _specialProcessButton == buttonList)) {
						buttonList->flags2 ^= 1;
						returnValue = buttonList->index | 0x8000;
						unk1 = true;
					}

					if (!(buttonList->flags & 4)) {
						buttonList->flags2 &= ~4;
						buttonList->flags2 &= ~2;
					}
					break;

				case 0x800:
					if (!(buttonList->flags & 4)) {
						buttonList->flags2 |= 4;
						buttonList->flags2 |= 2;
					}

					if (!(buttonList->flags & 1))
						unk1 = true;
					break;

				case 0x200:
					if (buttonList->flags & 4) {
						buttonList->flags2 |= 4;
						buttonList->flags2 |= 2;
					}

					if (!(buttonList->flags & 1))
						unk1 = true;
					break;

				case 0x100:
				default:
					buttonList->flags2 ^= 1;
					returnValue = buttonList->index | 0x8000;
					unk1 = true;
					if (buttonList->flags & 4) {
						buttonList->flags2 |= 4;
						buttonList->flags2 |= 2;
					}
					_specialProcessButton = buttonList;
				}
			}
		}

		bool unk2 = false;
		if ((flags & 0x2200) && progress) {
			buttonList->flags2 |= 6;
			if (!(buttonList->flags & 4) && !(buttonList->flags2 & 1)) {
				unk2 = true;
				buttonList->flags2 |= 1;
			}
		}

		if ((flags & 0x8800) == 0x8800) {
			_specialProcessButton = nullptr;
			if (!progress || (buttonList->flags & 4))
				buttonList->flags2 &= ~6;
		}

		if (!progress && buttonList == _specialProcessButton && !(buttonList->flags & 0x40))
			_specialProcessButton = nullptr;

		if ((buttonList->flags2 & 0x18) != ((buttonList->flags2 & 3) << 3))
			processButton(buttonList);

		if (unk2)
			buttonList->flags2 &= ~1;

		if (unk1) {
			buttonList->flags2 &= 0xFF;
			buttonList->flags2 |= flags;

			if (buttonList->buttonCallback) {
				_vm->removeInputTop();
				if ((*buttonList->buttonCallback)(buttonList))
					break;
			}

			if (buttonList->flags & 0x20)
				break;
		}

		if (_specialProcessButton == buttonList && (buttonList->flags & 0x40))
			break;

		buttonList = buttonList->nextButton;
	}

	if (!returnValue)
		returnValue = inputFlag & 0x7FFF;
	return returnValue;
}

void GUI_v2::updateButton(Button *button) {
	if (!button || (button->flags & 8))
		return;

	if (button->flags2 & 1)
		button->flags2 |= 8;
	else
		button->flags2 |= ~8;

	button->flags2 &= ~1;

	if (button->flags2 & 4)
		button->flags2 |= 0x10;
	else
		button->flags2 &= ~0x10;

	button->flags2 &= ~4;

	processButton(button);
}

void GUI_v2::getInput() {
	if (!_displayMenu)
		return;

	_vm->checkInput(_menuButtonList);
	_vm->removeInputTop();
	if (_vm->shouldQuit()) {
		_displayMenu = false;
		_isLoadMenu = false;
		_isSaveMenu = false;
		_isOptionsMenu = false;
		_isDeleteMenu = false;
	}

	_vm->delay(10);
}

void GUI_v2::renewHighlight(Menu &menu) {
	if (!_displayMenu)
		return;

	MenuItem &item = menu.item[menu.highlightedItem];
	int x = item.x + menu.x; int y = item.y + menu.y;
	int x2 = x + item.width - 1; int y2 = y + item.height - 1;
	redrawText(menu);
	_screen->fillRect(x + 2, y + 2, x2 - 2, y2 - 2, item.bkgdColor);
	redrawHighlight(menu);
	_screen->updateScreen();
}

void GUI_v2::backUpPage1(uint8 *buffer) {
	_screen->copyRegionToBuffer(1, 0, 0, 320, 200, buffer);
}

void GUI_v2::restorePage1(const uint8 *buffer) {
	_screen->copyBlockToPage(1, 0, 0, 320, 200, buffer);
}

void GUI_v2::setupSavegameNames(Menu &menu, int num) {
	for (int i = 0; i < num; ++i) {
		menu.item[i].useItemString = true;
		menu.item[i].itemString = "";
		menu.item[i].saveSlot = -1;
		menu.item[i].enabled = false;
	}

	int startSlot = 0;
	if (_isSaveMenu && _savegameOffset == 0)
		startSlot = 1;

	KyraEngine_v2::SaveHeader header;
	Common::InSaveFile *in;
	for (int i = startSlot; i < num && uint(_savegameOffset + i) < _saveSlots.size(); ++i) {
		if ((in = _vm->openSaveForReading(_vm->getSavegameFilename(_saveSlots[i + _savegameOffset]), header)) != nullptr) {
			Common::String s = header.description;
			s = Util::convertString_GUItoKYRA(s);

			if (_vm->gameFlags().lang == Common::JA_JPN || _vm->gameFlags().lang == Common::ZH_CHN || _vm->gameFlags().lang == Common::ZH_TWN) {
				// Strip special characters from GMM save dialog which might get misinterpreted as 2-byte characters
				for (Common::String::iterator ii = s.begin(); ii != s.end(); ++ii) {
					if (*ii < 32) // due to the signed char type this will also clean up everything >= 0x80
						*ii = ' ';
				}
			}

			// Trim long GMM save descriptions to fit our save slots
			_screen->_charSpacing = -2;
			int fC = _screen->getTextWidth(s.c_str());
			while (!s.empty() && fC > 240) {
				s.deleteLastChar();
				fC = _screen->getTextWidth(s.c_str());
			}
			_screen->_charSpacing = 0;

			menu.item[i].saveSlot = _saveSlots[i + _savegameOffset];
			menu.item[i].enabled = true;
			menu.item[i].useItemString = true;
			menu.item[i].itemString = s;
			delete in;
		}
	}

	if (_savegameOffset == 0) {
		if (_isSaveMenu) {
			menu.item[0].saveSlot = -2;
			menu.item[0].enabled = true;
			menu.item[0].useItemString = true;
			menu.item[0].itemString = getTableString(_vm->gameFlags().isTalkie ? 10 : 18);
		} else {
			menu.item[0].useItemString = true;
			menu.item[0].itemString = getTableString(_vm->gameFlags().isTalkie ? 34 : 42, _vm->gameFlags().lang == Common::RU_RUS);
		}
	}
}

int GUI_v2::scrollUpButton(Button *button) {
	updateMenuButton(button);

	if (_savegameOffset == (_isDeleteMenu ? 1 : 0))
		return 0;

	--_savegameOffset;
	if (_isLoadMenu) {
		setupSavegameNames(_loadMenu, _saveLoadNumSlots);
		// original calls something different here...
		initMenu(_loadMenu);
	} else if (_isSaveMenu || _isDeleteMenu) {
		setupSavegameNames(_saveMenu, _saveLoadNumSlots);
		// original calls something different here...
		initMenu(_saveMenu);
	}

	return 0;
}

int GUI_v2::scrollDownButton(Button *button) {
	updateMenuButton(button);
	++_savegameOffset;

	if (uint(_savegameOffset + _saveLoadNumSlots) >= _saveSlots.size())
		_savegameOffset = MAX<int>(_saveSlots.size() - _saveLoadNumSlots, _isDeleteMenu ? 1 : 0);

	if (_isLoadMenu) {
		setupSavegameNames(_loadMenu, _saveLoadNumSlots);
		// original calls something different here...
		initMenu(_loadMenu);
	} else if (_isSaveMenu || _isDeleteMenu) {
		setupSavegameNames(_saveMenu, _saveLoadNumSlots);
		// original calls something different here...
		initMenu(_saveMenu);
	}

	return 0;
}

int GUI_v2::resumeGame(Button *caller) {
	updateMenuButton(caller);
	_displayMenu = false;
	if (!(_vm->game() == GI_KYRA2 && _vm->gameFlags().lang == Common::ZH_TWN))
		_screen->setFontStyles(_screen->_currentFont, Font::kStyleBorder);
	return 0;
}

int GUI_v2::quitOptionsMenu(Button *caller) {
	updateMenuButton(caller);
	_isOptionsMenu = false;
	return 0;
}

int GUI_v2::toggleWalkspeed(Button *caller) {
	updateMenuButton(caller);
	if (_vm->_configWalkspeed == 5)
		_vm->_configWalkspeed = 3;
	else
		_vm->_configWalkspeed = 5;
	_vm->setWalkspeed(_vm->_configWalkspeed);
	setupOptionsButtons();
	renewHighlight(_gameOptions);
	return 0;
}

int GUI_v2::toggleText(Button *caller) {
	updateMenuButton(caller);

	if (_vm->textEnabled()) {
		if (_vm->speechEnabled())
			_vm->_configVoice = 1;
		else
			_vm->_configVoice = 3;
	} else {
		if (_vm->speechEnabled())
			_vm->_configVoice = 2;
		else
			_vm->_configVoice = 0;
	}

	setupOptionsButtons();
	renewHighlight(_gameOptions);
	return 0;
}

int GUI_v2::clickLoadSlot(Button *caller) {
	updateMenuButton(caller);

	int index = caller->index - _menuButtons[0].index;
	assert(index >= 0 && index <= 6);
	MenuItem &item = _loadMenu.item[index];

	if (item.saveSlot >= 0) {
		_vm->_gameToLoad = item.saveSlot;
		_isLoadMenu = false;
	}

	return 0;
}

int GUI_v2::cancelLoadMenu(Button *caller) {
	updateMenuButton(caller);
	_isLoadMenu = false;
	_noLoadProcess = true;
	return 0;
}

int GUI_v2::saveMenu(Button *caller) {
	updateSaveFileList(_vm->_targetName);

	updateMenuButton(caller);

	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);

	_isSaveMenu = true;
	_noSaveProcess = false;
	_saveSlot = -1;
	_savegameOffset = 0;
	setupSavegameNames(_saveMenu, _saveLoadNumSlots);
	initMenu(_saveMenu);

	updateAllMenuButtons();

	while (_isSaveMenu) {
		processHighlights(_saveMenu);
		getInput();
	}

	if (_noSaveProcess) {
		restorePage1(_vm->_screenBuffer);
		backUpPage1(_vm->_screenBuffer);
		initMenu(*_currentMenu);
		updateAllMenuButtons();
		return 0;
	} else if (_saveSlot <= -1) {
		return 0;
	}

	restorePage1(_vm->_screenBuffer);
	restorePalette();

	Graphics::Surface thumb;
	createScreenThumbnail(thumb);
	_vm->updatePlayTimer();
	Util::convertString_KYRAtoGUI(_saveDescription, 81);
	_vm->saveGameStateIntern(_saveSlot, _saveDescription, &thumb);
	thumb.free();

	_displayMenu = false;
	if (!(_vm->game() == GI_KYRA2 && _vm->gameFlags().lang == Common::ZH_TWN))
		_screen->setFontStyles(_screen->_currentFont, Font::kStyleBorder);
	_madeSave = true;

	return 0;
}

int GUI_v2::clickSaveSlot(Button *caller) {
	updateMenuButton(caller);

	int index = caller->index - _menuButtons[0].index;
	assert(index >= 0 && index <= 6);
	MenuItem &item = _saveMenu.item[index];

	if (item.saveSlot >= 0) {
		if (_isDeleteMenu) {
			_slotToDelete = item.saveSlot;
			_isDeleteMenu = false;
			return 0;
		} else {
			_saveSlot = item.saveSlot;
			Common::strlcpy(_saveDescription, item.itemString.c_str(), sizeof(_saveDescription));
		}
	} else if (item.saveSlot == -2) {
		_saveSlot = getNextSavegameSlot();
		memset(_saveDescription, 0, sizeof(_saveDescription));
	}

	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);

	initMenu(_savenameMenu);
	_screen->fillRect(0x26, 0x5B, 0x11F, _vm->gameFlags().lang == Common::ZH_TWN ? 0x6b : 0x66, textFieldColor2());
	g_system->setFeatureState(OSystem::kFeatureVirtualKeyboard, true);
	const char *desc = nameInputProcess(_saveDescription, 0x27, 0x5C, textFieldColor1(), textFieldColor2(), textFieldColor3(), 0x50);
	g_system->setFeatureState(OSystem::kFeatureVirtualKeyboard, false);
	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);
	if (desc) {
		_isSaveMenu = false;
		_isDeleteMenu = false;
	} else {
		initMenu(_saveMenu);
	}

	return 0;
}

int GUI_v2::cancelSaveMenu(Button *caller) {
	updateMenuButton(caller);
	_isSaveMenu = false;
	_isDeleteMenu = false;
	_noSaveProcess = true;
	return 0;
}

int GUI_v2::deleteMenu(Button *caller) {
	updateSaveFileList(_vm->_targetName);

	updateMenuButton(caller);
	if (_saveSlots.size() < 2) {
		_vm->snd_playSoundEffect(0x0D);
		return 0;
	}

	do {
		restorePage1(_vm->_screenBuffer);
		backUpPage1(_vm->_screenBuffer);
		_savegameOffset = 1;
		_saveMenu.menuNameId = _vm->gameFlags().isTalkie ? 35 : 1;
		setupSavegameNames(_saveMenu, _saveLoadNumSlots);
		initMenu(_saveMenu);
		_isDeleteMenu = true;
		_slotToDelete = -1;
		updateAllMenuButtons();

		while (_isDeleteMenu) {
			processHighlights(_saveMenu);
			getInput();
		}

		if (_slotToDelete < 1) {
			restorePage1(_vm->_screenBuffer);
			backUpPage1(_vm->_screenBuffer);
			initMenu(*_currentMenu);
			updateAllMenuButtons();
			_saveMenu.menuNameId = _vm->gameFlags().isTalkie ? 9 : 17;
			return 0;
		}
	} while (choiceDialog(_vm->gameFlags().isTalkie ? 0x24 : 2, 1) == 0);

	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);
	initMenu(*_currentMenu);
	updateAllMenuButtons();
	_vm->_saveFileMan->removeSavefile(_vm->getSavegameFilename(_slotToDelete));
	Common::Array<int>::iterator i = Common::find(_saveSlots.begin(), _saveSlots.end(), _slotToDelete);
	while (i != _saveSlots.end()) {
		++i;
		if (i == _saveSlots.end())
			break;
		// We are only renaming all savefiles until we get some slots missing
		// Also not rename quicksave slot filenames
		if (*(i - 1) != *i || *i >= 990)
			break;
		Common::String oldName = _vm->getSavegameFilename(*i);
		Common::String newName = _vm->getSavegameFilename(*i - 1);
		_vm->_saveFileMan->renameSavefile(oldName, newName);
	}
	_saveMenu.menuNameId = _vm->gameFlags().isTalkie ? 9 : 17;
	return 0;
}

const char *GUI_v2::nameInputProcess(char *buffer, int x, int y, uint8 c1, uint8 c2, uint8 c3, int bufferSize) {
	bool running = true;
	int curPos = strlen(buffer);
	uint8 keyLim = (_vm->gameFlags().lang == Common::JA_JPN || _vm->gameFlags().lang == Common::ZH_TWN) ? 128 : 226;
	int x2 = x, y2 = y;

	Screen::FontId of = _screen->setFont(_saveMenuFont);
	_text->printText(buffer, x, y, c1, c2, c2);

	for (int i = 0; i < curPos; ++i)
		x2 += getCharWidth(buffer[i]);

	drawTextfieldBlock(x2, y2, c3);
	_screen->setFont(of);

	_keyPressed.reset();
	_cancelNameInput = _finishNameInput = false;
	while (running && !_vm->shouldQuit()) {
		of = _screen->setFont(_saveMenuFont);
		checkTextfieldInput();
		_screen->setFont(of);
		processHighlights(_savenameMenu);

		char inputKey = _keyPressed.ascii;
		Util::convertISOToDOS(inputKey);

		if (_keyPressed.keycode == Common::KEYCODE_RETURN || _keyPressed.keycode == Common::KEYCODE_KP_ENTER || _finishNameInput) {
			if (checkSavegameDescription(buffer, curPos)) {
				buffer[curPos] = 0;
				running = false;
			} else {
				_finishNameInput = false;
			}
		} else if (_keyPressed.keycode == Common::KEYCODE_ESCAPE || _cancelNameInput) {
			running = false;
			return nullptr;
		} else if ((_keyPressed.keycode == Common::KEYCODE_BACKSPACE || _keyPressed.keycode == Common::KEYCODE_DELETE) && curPos > 0) {
			drawTextfieldBlock(x2, y2, c2);
			--curPos;
			x2 -= getCharWidth(buffer[curPos]);
			drawTextfieldBlock(x2, y2, c3);
			_screen->updateScreen();
			_lastScreenUpdate = _vm->_system->getMillis();
		} else if ((uint8)inputKey > 31 && (uint8)inputKey < keyLim && curPos < bufferSize) {
			of = _screen->setFont(_saveMenuFont);
			if (x2 + getCharWidth(inputKey) + 7 < 0x11F) {
				buffer[curPos] = inputKey;
				const char text[2] = { buffer[curPos], 0 };
				if (_saveMenuFont == Screen::FID_CHINESE_FNT) {
					drawTextfieldBlock(x2, y2, c2);
					_text->printText(text, x2, y2, c1, c2, 0);
				} else {
					_text->printText(text, x2, y2, c1, c2, c2);
				}
				x2 += getCharWidth(inputKey);
				drawTextfieldBlock(x2, y2, c3);
				++curPos;
				_screen->updateScreen();
				_lastScreenUpdate = _vm->_system->getMillis();
			}
			_screen->setFont(of);
		}

		_keyPressed.reset();
	}

	return buffer;
}

int GUI_v2::finishSavename(Button *caller) {
	updateMenuButton(caller);
	_finishNameInput = true;
	return 0;
}

int GUI_v2::cancelSavename(Button *caller) {
	updateMenuButton(caller);
	_cancelNameInput = true;
	return 0;
}

bool GUI_v2::checkSavegameDescription(const char *buffer, int size) {
	if (!buffer || !size)
		return false;
	if (buffer[0] == 0)
		return false;
	for (int i = 0; i < size; ++i) {
		if (buffer[i] != 0x20)
			return true;
		else if (buffer[i] == 0x00)
			return false;
	}
	return false;
}

int GUI_v2::getCharWidth(uint8 c) {
	Screen::FontId old = _screen->setFont(_saveMenuFont);
	_screen->_charSpacing = -2;
	int width = _screen->getCharWidth(c);
	_screen->_charSpacing = 0;
	_screen->setFont(old);
	return width;
}

void GUI_v2::drawTextfieldBlock(int x, int y, uint8 c) {
	_screen->fillRect(x + _saveMenuCursor.left, y + _saveMenuCursor.top, x + _saveMenuCursor.right, y + _saveMenuCursor.bottom, c);
}

bool GUI_v2::choiceDialog(int name, bool type) {
	_choiceMenu.highlightedItem = 0;
	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);
	if (type)
		_choiceMenu.numberOfItems = 2;
	else
		_choiceMenu.numberOfItems = 1;
	_choiceMenu.menuNameId = name;

	initMenu(_choiceMenu);
	_isChoiceMenu = true;
	_choice = false;

	while (_isChoiceMenu) {
		processHighlights(_choiceMenu);
		getInput();
	}

	restorePage1(_vm->_screenBuffer);
	backUpPage1(_vm->_screenBuffer);
	return _choice;
}

int GUI_v2::choiceYes(Button *caller) {
	updateMenuButton(caller);
	_choice = true;
	_isChoiceMenu = false;
	return 0;
}

int GUI_v2::choiceNo(Button *caller) {
	updateMenuButton(caller);
	_choice = false;
	_isChoiceMenu = false;
	return 0;
}

} // End of namespace Kyra
