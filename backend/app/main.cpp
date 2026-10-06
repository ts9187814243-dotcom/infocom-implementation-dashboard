#include "httplib.h"

int main() {
	httplib::Server svr;
	svr.Get("/health", [](const auto&, auto& res) {
		res.set_content("{\status\":\"ok\"}", "application/json");
		});
	svr.listen("0.0.0.0", 8080);
}
