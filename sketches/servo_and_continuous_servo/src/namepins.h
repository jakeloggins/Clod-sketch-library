#define PIN_A 2
String thisDeviceName = "backToBasic";
String thisDevicePath = "/esp";
String subscribe_path = "/esp/#";
String lookup(String endpoint_key) {
	if (endpoint_key == "alarmLight") {
		return "RGB";
	}
}
