#include "sysinclude.h"
#include "deviceint.h"
#include "vmips.h"

extern vmips *machine;

char *DeviceInt::strlineno(uint32 line)
{
	static char buff[50];

	if (line == 0x00008000) { sprintf(buff, "IRQ7"); }
	else if (line == 0x00004000) { sprintf(buff, "IRQ6"); }
	else if (line == 0x00002000) { sprintf(buff, "IRQ5"); }
	else if (line == 0x00001000) { sprintf(buff, "IRQ4"); }
	else if (line == 0x00000800) { sprintf(buff, "IRQ3"); }
	else if (line == 0x00000400) { sprintf(buff, "IRQ2"); }
	else if (line == 0x00000200) { sprintf(buff, "IRQ1"); }
	else if (line == 0x00000100) { sprintf(buff, "IRQ0"); }
	else { sprintf(buff, "something strange (0x%08lx)", line); }
	return buff;
}

void DeviceInt::reportAssert(uint32 line)
{
	if (machine->opt->option("reportirq")->flag)
		fprintf(stderr, "%s asserted %s\n", descriptor_str(), strlineno(line));
}

void DeviceInt::reportAssertDisconnected(uint32 line)
{
	if (machine->opt->option("reportirq")->flag)
		fprintf(stderr, "%s asserted %s but it wasn't connected\n",
			descriptor_str(), strlineno(line));
}

void DeviceInt::reportDeassert(uint32 line)
{
	if (machine->opt->option("reportirq")->flag)
		fprintf(stderr, "%s deasserted %s\n", descriptor_str(),
			strlineno(line));
}

void DeviceInt::reportDeassertDisconnected(uint32 line)
{
	if (machine->opt->option("reportirq")->flag)
		fprintf(stderr, "%s deasserted %s but it wasn't connected\n",
			descriptor_str(), strlineno(line));
}

void DeviceInt::assertInt(uint32 line)
{
	if (line & lines_connected) {
		if (! (line & lines_asserted)) reportAssert(line);
		lines_asserted |= line;
	} else {
		reportAssertDisconnected(line);
	}
}

void DeviceInt::deassertInt(uint32 line)
{
	if (line & lines_connected) {
		if (line & lines_asserted) reportDeassert(line);
		lines_asserted &= ~line;
	} else {
		reportDeassertDisconnected(line);
	}
}

bool DeviceInt::isAsserted(uint32 line)
{
	return (line & lines_connected) && (line & lines_asserted);
}

DeviceInt::DeviceInt()
{
	lines_connected = lines_asserted = 0;
	next = NULL;
}

