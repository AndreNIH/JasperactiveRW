#include "tcp_server.h"
#include "man_memory.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <regex>
#include "curl/curl.h"
#include "curl/easy.h"

#pragma comment(lib, "Ws2_32.lib")
#ifdef _DEBUG
#pragma comment(lib,"libcurld.lib")
#else
#pragma comment(lib,"libcurl.lib")
#endif
const std::vector<BYTE> StrToVec(const std::string& source, bool u_encode = false) {
	std::vector<BYTE> vec;
	for (auto c : source) {
		vec.push_back(c);
		if (u_encode) vec.push_back('\0');
	}
	return vec;
}


const std::string passExam(std::string interceptedData) {
	interceptedData = std::regex_replace(interceptedData, std::regex((R"(<(a:dCorrectPercentage)>(.*?)<\/\1>)")),"<a:dCorrectPercentage>1</a:dCorrectPercentage>");
	interceptedData = std::regex_replace(interceptedData, std::regex(R"(<(a:bCorrect)>(false)(<\/\1>))"), "<a:bCorrect>true</a:bCorrect>");
	short correctCount = 0;
	size_t next = 0;
	do {
		next = interceptedData.find("true", next);
		if (next != std::string::npos) { next++; correctCount++; }
	}while (next != std::string::npos);
	const std::string newcorrect = "<a:iCorrects>" + std::to_string(correctCount) + "</a:iCorrects>";
	return std::regex_replace(interceptedData, std::regex(R"(<(a:iCorrects)>(\d*?)(<\/\1>))"), newcorrect);;
}
size_t curl_callback(char* response, size_t size, size_t nmemb, void* userdata) {
	*reinterpret_cast<std::string*>(userdata) += response;
	return nmemb * size;
}

void server_callback(SERVER_TCP *invoker, int port,std::pair<void*,std::string>(recieved)) {
	std::string ExamenCompletado = passExam(recieved.second);
	CURL* cc = curl_easy_init();
	struct curl_slist* headers = NULL;
	if (cc == 0) throw std::runtime_error{ "Error al inicializar Libcurl" };
	else {
		std::string ServerResponse;
		curl_easy_setopt(cc, CURLOPT_URL, "http://ja-web-service.cloudapp.net/Jasperactive.svc");
		curl_easy_setopt(cc, CURLOPT_POSTFIELDSIZE, ExamenCompletado.size());
		curl_easy_setopt(cc, CURLOPT_COPYPOSTFIELDS, ExamenCompletado.c_str());
		curl_easy_setopt(cc, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
		curl_easy_setopt(cc, CURLOPT_WRITEFUNCTION, &curl_callback);
		curl_easy_setopt(cc, CURLOPT_WRITEDATA, &ServerResponse);
		for (auto& header : { "Content-Type: text/xml; charset=utf-8","Connection: Keep-alive","SOAPAction: \"http://tempuri.org/IJasperactive/SaveResults\"" }) headers = curl_slist_append(headers, header);
		curl_easy_setopt(cc, CURLOPT_HTTPHEADER, headers);
		std::cout << ServerResponse;
		invoker->sendResponse(port,ServerResponse);
		curl_easy_cleanup(cc);
	}


}

int	main() {
	WSADATA ws{ 0 };
	if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
		std::cout << "Error al inicializar Winsocks2\n";
		return 0;
	}

	SERVER_TCP node("127.0.0.1", 80, server_callback);
	Memory ja_client;
	
	std::cout << "Esperando a Jasperactive...\n";
	do {
		ja_client.bind("LMSOfficeApp.exe");
	} while (!ja_client.isBound());
	std::cout << "Localizado!\n";

	bool alreadyPatched = false;

	auto jasperactivedotcom = StrToVec("http://ja-web-service.cloudapp.net/", true);
	auto localhost = StrToVec("http://127.0.0.1/", true);
	for (int i = 0; i < 30; i++) localhost.push_back(0); //Padding
	
	DWORD SS_ADDR = 0;
	for (auto &target : { "http://ja-web-service.cloudapp.net/","http://127.0.0.1/" }) {
		SS_ADDR = ja_client.search_bytes(StrToVec(target,true));
		if (SS_ADDR != 0) break;
		else alreadyPatched = true;
	}
	if (!SS_ADDR) return 0;
	
	if(!alreadyPatched) if(!ja_client.Write(SS_ADDR,localhost)) return 0;
	std::cout << "Memoria reescrita. Intercepcion de paquetes activa\n";
	try {
		node.startServer();
	}
	catch(std::exception e){
		std::cout << e.what();
		ja_client.Write(SS_ADDR, StrToVec("http://ja-web-service.cloudapp.net/", true));
		WSACleanup();
		return 0;
	}
	if (ja_client.Write(SS_ADDR, StrToVec("http://ja-web-service.cloudapp.net/", true))) std::cout << "Proceso finalizado con exito...\n";
	else std::cout << "Solicitud procesada\n[ADVERTENCIA] No se puedo sobreescribir memoria en el proceso. Espere comportamiento indefinido\n\n===PROCESO FINALIZADO CON ERRORES===\n";
	WSACleanup();
	return 0;
}
