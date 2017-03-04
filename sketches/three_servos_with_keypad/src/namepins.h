#define PIN_A 2
#define PIN_B 5
#define PIN_C 12
#define PIN_D 13
String thisDeviceName = "backToBasic";
String thisDevicePath = "/esp";
String subscribe_path = "/esp/#";
String lookup(String endpoint_key) {
	if (endpoint_key == "alarmLight") {
		return "RGB";
	}
}
