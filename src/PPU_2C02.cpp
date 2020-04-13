#include "fmt/printf.h"

#include "PPU_2C02.h"


PPU_2C02::PPU_2C02() : m_size(8) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fmt::print("Couldn't initialize SDL video!");
		return;
	}

	m_window = SDL_CreateWindow(
		"POCNESEMU", 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1024, 960, NULL
	);

	m_renderer = SDL_CreateRenderer(m_window, -1, 0);
	m_screen = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING, 256, 240);
	m_screenBuffer = new SDL_Color[256 * 240];
	
	m_patternWindow = SDL_CreateWindow(
		"POCNESEMU - Pattern Tables",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		512, 240, SDL_WINDOW_HIDDEN
	);

	m_patternRenderer = SDL_CreateRenderer(m_patternWindow, 0, 0);
	m_patternScreen = SDL_CreateTexture(m_patternRenderer, SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING, 256, 128);
	m_patternBuffer = new SDL_Color[256 * 128];

	// Set "At Power" internal state
	PPU_CTRL.data = 0x00;
	PPU_MASK.data = 0x00;
	PPU_STATUS.data = 0xA0;
	addressLatch = 0;
	ppuAddress = 0x0000;
	dataBuffer = 0x00;
}


PPU_2C02::~PPU_2C02() {
	if (m_renderer != nullptr)
		SDL_DestroyRenderer(m_renderer);
	if (m_window != nullptr)
		SDL_DestroyWindow(m_window);
	if (m_screen != nullptr)
		SDL_DestroyTexture(m_screen);
	if (m_screenBuffer != nullptr)
		delete[] m_screenBuffer;
	if (m_patternRenderer != nullptr)
		SDL_DestroyRenderer(m_patternRenderer);
	if (m_patternWindow != nullptr)
		SDL_DestroyWindow(m_patternWindow);
	if (m_patternScreen != nullptr)
		SDL_DestroyTexture(m_patternScreen);
	if (m_patternBuffer != nullptr)
		delete[] m_patternBuffer;

	SDL_Quit();
}


bool PPU_2C02::isRunning() {
	return m_isRunning;
}

bool PPU_2C02::getNmi() {
	return m_nmi;
}

void PPU_2C02::clearNmi() {
	m_nmi = false;
	PPU_CTRL.enableNmi = 0;
}

int PPU_2C02::getCycle() {
	return m_cycle;
}

int PPU_2C02::getScanline() {
	return m_scanline;
}


void PPU_2C02::connectBus(std::shared_ptr<IBus<uint16_t, uint8_t>> bus) {
	m_bus = bus;
	m_busConnected = true;
}


void PPU_2C02::reset() {
	// Set "After Reset" internal state
	PPU_CTRL.data = 0x00;
	PPU_MASK.data = 0x00;
	PPU_STATUS.data = 0xA0;
	addressLatch = 0;
	dataBuffer = 0x00;

	m_cycle = 0;
	m_scanline = 0;
	m_isRunning = true;

	// Initialize screen buffer with a value and render it
	for (int i = 0; i < 256 * 240; i++) {
		m_screenBuffer[i] = m_palette[0x0F];
	}
	
	for (int i = 0; i < 256 * 128; i++) {
		m_patternBuffer[i] = m_palette[0x0F];
	}

	SDL_UpdateTexture(m_screen, NULL, m_screenBuffer, sizeof(SDL_Color) * 256);
	SDL_RenderCopy(m_renderer, m_screen, NULL, NULL);
	SDL_RenderPresent(m_renderer);


	SDL_UpdateTexture(m_patternScreen, NULL, m_patternBuffer, sizeof(SDL_Color) * 256);
	SDL_RenderCopy(m_patternRenderer, m_patternScreen, NULL, NULL);
	SDL_RenderPresent(m_patternRenderer);
}


void PPU_2C02::tick() {
	// Set VBlank and NMI
	if (m_scanline == -1 && m_cycle == 1)
		PPU_STATUS.verticalBlank = 0;

	if (m_scanline == 241 && m_cycle == 1)
		PPU_STATUS.verticalBlank = 1;
	
	if (PPU_STATUS.verticalBlank && PPU_CTRL.enableNmi) {
		m_nmi = true;
	}


	// Reset Cycles and Scanlines
	if (m_cycle >= 341) {
 		m_cycle = 0;
 		m_scanline++;
 		if (m_scanline >= 261) {
 			m_scanline = -1;
 			m_frameComplete = true;
 		}
	}


	// Render background pixel
	if (PPU_MASK.bgShow) {
		if (m_cycle < 256 && m_scanline < 240 && m_scanline != -1) {
			const int tileX = m_cycle / 8;
			const int tileY = m_scanline / 8;
			const int col = m_cycle % 8;
			const int row = m_scanline % 8;
			const int index = tileY * 32 + tileX;

			// Run this per tile
			if (col == 0) {
				m_tile_lsb = readFrom(PPU_CTRL.bgPattern * 0x1000
					+ readFrom(0x2000 + index) * 16 + row + 0);
				m_tile_msb = readFrom(PPU_CTRL.bgPattern * 0x1000
					+ readFrom(0x2000 + index) * 16 + row + 8);


				if ((tileX & 0x03) < 2 && (tileY & 0x03) < 2) {
					// top left
					m_shift = 0;
					m_pos = 0b00000011;
				}
				else if ((tileX & 0x03) >= 2 && (tileY & 0x03) < 2) {
					// top right
					m_shift = 2;
					m_pos = 0b00001100;
				}
				else if ((tileX & 0x03) < 2 && (tileY & 0x03) >= 2) {
					// bottom left
					m_shift = 4;
					m_pos = 0b00110000;
				}
				else if ((tileX & 0x03) >= 2 && (tileY & 0x03) >= 2) {
					// bottom right
					m_shift = 6;
					m_pos = 0b11000000;
				}
			}

			// Set color for the specific pixel
			m_screenBuffer[m_cycle + m_scanline * 256]
				= m_palette[
					readFrom(
						0x3F00 +
						(((readFrom(0x23C0 + tileX / 4 + (tileY / 4) * 8) & m_pos) >> m_shift) << 2)
						+ ((m_tile_lsb & (0x80 >> col)) != 0) | (((m_tile_msb & (0x80 >> col)) != 0) << 1)
					) & (PPU_MASK.greyScale ? 0x30 : 0xFF)
				];
		}
	}


	// Render sprites
	if (PPU_MASK.sprShow)
		void;


	// Run this per frame
	if (m_frameComplete) {
		// Render pattern memory
		for (int tileY = 0; tileY < 16; tileY++) {
			for (int tileX = 0; tileX < 16; tileX++) {
				int offset = tileY * 256 + tileX * 16;

				for (int row = 0; row < 8; row++) {
					int tile_lsb = readFrom(offset + row + 0);
					int tile_msb = readFrom(offset + row + 8);

					for (int col = 0; col < 8; col++) {
						uint8_t pixel = (tile_lsb & 0x01) | ((tile_msb & 0x01) << 1);
						tile_lsb >>= 1; tile_msb >>= 1;

						m_patternBuffer[tileX * 8 + (7 - col) +
							(tileY * 8 + row) * 256] = 
							m_palette[
								readFrom(0x3F00 + (m_selectedPalette << 2) + pixel)
							];;
					}
				}
			}
		}

		for (int tileY = 0; tileY < 16; tileY++) {
			for (int tileX = 0; tileX < 16; tileX++) {
				int offset = tileY * 256 + tileX * 16;

				for (int row = 0; row < 8; row++) {
					int tile_lsb = readFrom(0x1000 + offset + row + 0);
					int tile_msb = readFrom(0x1000 + offset + row + 8);

					for (int col = 0; col < 8; col++) {
						uint8_t pixel = (tile_lsb & 0x01) | ((tile_msb & 0x01) << 1);
						tile_lsb >>= 1; tile_msb >>= 1;

						m_patternBuffer[128 + tileX * 8 + (7 - col) +
							(tileY * 8 + row) * 256] =
							m_palette[
								readFrom(0x3F00 + (m_selectedPalette << 2) + pixel)
							];
					}
				}
			}
		}


		// Poll Events
		SDL_PollEvent(&m_event);
		if (m_event.type == SDL_WINDOWEVENT
			&& m_event.window.event == SDL_WINDOWEVENT_CLOSE) {
			if (SDL_GetWindowID(m_window) == m_event.window.windowID) {
				m_isRunning = false;
			} 
			if (SDL_GetWindowID(m_patternWindow) == m_event.window.windowID) {
				SDL_HideWindow(m_patternWindow);
			}
		}
		else if (m_event.type == SDL_KEYDOWN) {
			if (m_event.key.keysym.sym == SDLK_LEFTBRACKET)
				--m_selectedPalette &= 0x07;
			if (m_event.key.keysym.sym == SDLK_RIGHTBRACKET)
				++m_selectedPalette &= 0x07;
			if (m_event.key.keysym.sym == SDLK_p)
				SDL_ShowWindow(m_patternWindow);
		}


		// Render Frame
		SDL_UpdateTexture(m_screen, NULL, m_screenBuffer, sizeof(SDL_Color) * 256);
		SDL_RenderCopy(m_renderer, m_screen, NULL, NULL);
		SDL_RenderPresent(m_renderer);

		SDL_UpdateTexture(m_patternScreen, NULL, m_patternBuffer,
			sizeof(SDL_Color) * 256);
		SDL_RenderCopy(m_patternRenderer, m_patternScreen, NULL, NULL);
		SDL_RenderCopy(m_patternRenderer, m_patternScreen, NULL, NULL);
		SDL_RenderPresent(m_patternRenderer);

		m_frameComplete = false;
	}

	m_cycle++;
}


uint8_t PPU_2C02::read(uint16_t address, bool readOnly) {
	switch(address % m_size) {
	case 0x0000:	// Control
		if (readOnly)
			return PPU_CTRL.data;
		break;
	case 0x0001:	// Mask
		if (readOnly)
			return PPU_MASK.data;
		break;
	case 0x0002:	// Status
		m_tempData = PPU_STATUS.data;

		if (!readOnly) {
			PPU_STATUS.verticalBlank = 0;
			addressLatch = 0;
		}

		//	Return top 3 bits, the rest are residual data
		return (m_tempData & 0xE0) | (dataBuffer & 0x1F);

	case 0x0003:	// OAM Address
		break;
	case 0x0004:	// OAM Data
		break;
	case 0x0005:	// Scroll
		break;
	case 0x0006:	// PPU Address
		break;
	case 0x0007:	// PPU Data
		if (readOnly)
			return dataBuffer;

		m_tempData = dataBuffer;
		dataBuffer = readFrom(ppuAddress);

		// Pallette reads are not delayed
		if (ppuAddress >= 0x3F00) {
			ppuAddress++;
			return dataBuffer;
		}

		ppuAddress++;
		return m_tempData;
	}

	return -1;

}


void PPU_2C02::write(uint16_t address, uint8_t data) {
	switch (address % m_size) {
	case 0x0000:	// Control
		PPU_CTRL.data = data;

		break;
	case 0x0001:	// Mask
		PPU_MASK.data = data;

		break;
	case 0x0002:	// Status
		break;

	case 0x0003:	// OAM Address
		break;

	case 0x0004:	// OAM Data
		break;

	case 0x0005:	// Scroll
		break;

	case 0x0006:	// PPU Address
		if (addressLatch == 0) {
			ppuAddress = (ppuAddress & 0x00FF) | (data << 8);
			addressLatch = 1;
		}
		else {
			ppuAddress = (ppuAddress & 0xFF00) | data;
			addressLatch = 0;
		}

		break;
	case 0x0007:	// PPU Data
		writeTo(ppuAddress, data);
		ppuAddress += (PPU_CTRL.incrementMode ? 32 : 1);

		break;

	}
}
