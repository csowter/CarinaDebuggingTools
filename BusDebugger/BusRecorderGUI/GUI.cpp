#include "GUI.h"

#include "SourceCodeProFont.h"

#include "ValuePacketValues.h"

#include <algorithm>
#include <bitset>
#include <iomanip>
#include <sstream>

using namespace std;

GUI::GUI()
	: app{"Bus Recorder", 0, 0, windowWidth, windowHeight, false}
{
	app.font = sdl::fonts::ttf_font{SourceCodePro_Medium_ttf,
									SourceCodePro_Medium_ttf_len, 20};
	app.colour({0x00, 0x00, 0x00, 0xFF});
	SetLabelPositions();

	int x = 0;
	int y = 0;

	for (int i = 0; i < NUMBER_OF_BUS_TIMESLOTS; ++i)
	{
		if ((i != 0) &&
			(i % (NUMBER_OF_BUS_TIMESLOTS / numberOfTimeslotRows) == 0))
		{
			x = 0;
			y += timeslotHeight;
		}

		timeslot[i].SetLocation(x, y);
		timeslot[i].InitialiseLabels(i);

		x += (windowWidth / (NUMBER_OF_BUS_TIMESLOTS / numberOfTimeslotRows));
	}

	logLines.colour(validColour);
	mapLines.colour(validColour);

	mapLabel.colour(validColour);
	mapLabel.location.y = mapLabelTop;
	andLabel.colour(validColour);
	andLabel.location.y = andLabelTop;
	orLabel.colour(validColour);
	orLabel.location.y = orLabelTop;

	timeslot[0].AddModuleTypeToSlot("R");
	timeslot[1].AddModuleTypeToSlot("A");
	timeslot[2].AddModuleTypeToSlot("A");
	timeslot[3].AddModuleTypeToSlot("A");
	timeslot[4].AddModuleTypeToSlot("A");
	timeslot[5].AddModuleTypeToSlot("B");
	timeslot[6].AddModuleTypeToSlot("D");
	timeslot[7].AddModuleTypeToSlot("D");
	timeslot[8].AddModuleTypeToSlot("D");
	timeslot[9].AddModuleTypeToSlot("D");
	timeslot[19].AddModuleTypeToSlot("Q");
}

void GUI::SetLabelPositions()
{
	int y = logLabelTop;
	for_each(begin(logLabels), end(logLabels), [&y](sdl::widgets::label& p) {
		p.location.y = y;
		y += labelYSpacing;
	});
}

void GUI::UpdateWidgets()
{
	auto label = begin(logLabels);
	auto text = begin(logText);
	auto colour = begin(logTextColour);
	for (; label != end(logLabels); ++label, ++text, ++colour)
	{
		label->text_and_colour(*text, *colour);
	}

	mapLabel.text(mapText);
	andLabel.text(andText);
	orLabel.text(orText);

	for (auto& t : timeslot)
	{
		t.UpdateWidgets();
	}
}

void GUI::AddLogEntry(string text, SDL_Color colour)
{
	logText[0] = move(text);
	logTextColour[0] = colour;
	rotate(begin(logText), begin(logText) + 1, end(logText));
	rotate(begin(logTextColour), begin(logTextColour) + 1, end(logTextColour));
}

void GUI::NonPacketBytesReceived(unsigned char* bytes, unsigned int length,
								 uint32_t time)
{
	AddLogEntry("Bytes");
}

void GUI::PacketReceived(BusHeader* header, unsigned int totalPacketLength,
						 uint32_t time)
{
	stringstream s;
	SDL_Color colour = validColour;

	++packetCount;
	s << setw(8) << setfill('0') << packetCount << " : ";

	switch (header->packetType)
	{
	case BPT_NORMAL_SYNC:
		s << "Sync            ";
		ProcessNormalSyncPacket(reinterpret_cast<BusSyncPacket*>(header));
		colour = {0x00, 0xAA, 0x00, 0xFF};
		break;
	case BPT_VALUES:
		s << "Values          ";
		ProcessValuesPacket(reinterpret_cast<BusValuesPacketPreamble*>(header));
		break;
	case BPT_TIMESLOT_IN_USE:
		s << "Time slot in use";
		break;
	default:
		s << "Unknown packet type : " << static_cast<int>(header->packetType);
		colour = invalidColour;
		break;
	}

	AddLogEntry(s.str(), colour);
}

void GUI::PacketWithInvalidCrcReceived(BusHeader* header, uint32_t time)
{
	AddLogEntry("Invalid packet");
}

void GUI::ProcessNormalSyncPacket(BusSyncPacket* packet)
{
	for (unsigned int i = 0; i < NUMBER_OF_BUS_TIMESLOTS; ++i)
	{
		timeslot[i].active = (packet->data.map & (1u << i));
	}

	{
		uint32_t mask = (1 << NUMBER_OF_BUS_TIMESLOTS) - 1;
		stringstream s;
		bitset<NUMBER_OF_BUS_TIMESLOTS> mapBits{packet->data.map & mask};
		s << "Map : " << mapBits;
		mapText = s.str();
	}

	{
		stringstream s;
		bitset<32> andBits{packet->data.andValues};
		s << "And : " << andBits;
		andText = s.str();
	}

	{
		stringstream s;
		bitset<32> orBits{packet->data.orValues};
		s << "Or  : " << orBits;
		orText = s.str();
	}
}

#include "Modules/TypeR/ConvertedProject_R_Rx_IDs.h"

void GUI::ProcessValuesPacket(BusValuesPacketPreamble* packet)
{
	auto const& slot = packet->data.slot;
	unsigned int i;
	for (i = 0; i < NUMBER_OF_BUS_TIMESLOTS; ++i)
	{
		if (slot & (1u << i))
		{
			timeslot[i].IncrementPacketCount();
			break;
		}
	}

	if (i < NUMBER_OF_BUS_TIMESLOTS)
	{
		uint16_t const required = 291 - 177;
		uint16_t const requiredUpper = required + 4;

		for (auto& value : packet)
		{
			auto const index = (value.index & 0x7FFFu);
			switch (index)
			{
			case BUS_DIAGNOSTICS_TYPE_A_WHAT_I_CAN_SEE:
			case BUS_DIAGNOSTICS_TYPE_B_WHAT_I_CAN_SEE:
			case BUS_DIAGNOSTICS_TYPE_D_WHAT_I_CAN_SEE:
			case BUS_DIAGNOSTICS_TYPE_R_WHAT_I_CAN_SEE:
			case BUS_DIAGNOSTICS_TYPE_E_WHAT_I_CAN_SEE:
			case BUS_DIAGNOSTICS_TYPE_F_WHAT_I_CAN_SEE:
				timeslot[i].whatICanSeeText = "WICS " + to_string(value.value);
				break;
			case BUS_VARIANTS_R:
			case BUS_VARIANTS_A:
			case BUS_VARIANTS_B:
			case BUS_VARIANTS_D:
			case BUS_VARIANTS_K:
			case BUS_VARIANTS_E:
			case BUS_VARIANTS_Q:
			case BUS_VARIANTS_F:
				timeslot[i].variantText = "Variant " + to_string(value.value);
				break;
			}
		}
	}
}
