/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef PROTOCOLMODULE_H
#define PROTOCOLMODULE_H

#include <QString>
#include <QLibrary>
#include <QIODevice>
#include <QRegExp>
#include <string>
#include <unordered_map>
#include <memory>

class ProtocolModule{
	QLibrary lib;
	bool ok;
	std::string protocol;

	class protocol_client_t;
	class unknown_stream_t;
	class file_enumerator_t;

	typedef const char *(*get_protocol_f)();
	typedef protocol_client_t *(*initialize_client_f)(const wchar_t *, const wchar_t *);
	typedef void (*terminate_client_f)(protocol_client_t *);
	typedef unknown_stream_t *(*open_file_utf16_f)(protocol_client_t *, const wchar_t *);
	typedef void (*close_file_f)(unknown_stream_t *);
	typedef std::uint64_t(*read_file_f)(unknown_stream_t *, void *, std::uint64_t);
	typedef file_enumerator_t *(*create_file_enumerator_f)(protocol_client_t *, const wchar_t *);
	typedef const wchar_t *(*file_enumerator_next_f)(file_enumerator_t *);
	typedef void (*destroy_file_enumerator_f)(file_enumerator_t *);
#define DECLARE_FUNCTION_POINTER(x) x##_f x
	DECLARE_FUNCTION_POINTER(get_protocol);
	DECLARE_FUNCTION_POINTER(initialize_client);
	DECLARE_FUNCTION_POINTER(terminate_client);
	DECLARE_FUNCTION_POINTER(open_file_utf16);
	DECLARE_FUNCTION_POINTER(close_file);
	DECLARE_FUNCTION_POINTER(read_file);
	DECLARE_FUNCTION_POINTER(create_file_enumerator);
	DECLARE_FUNCTION_POINTER(file_enumerator_next);
	DECLARE_FUNCTION_POINTER(destroy_file_enumerator);
	protocol_client_t *client;

	class Stream : public QIODevice{
		ProtocolModule *module;
		unknown_stream_t *stream;
	public:
		Stream(ProtocolModule *module, unknown_stream_t *stream);
		~Stream();
		qint64 readData(char *data, qint64 maxSize) override;
		qint64 writeData(const char *data, qint64 maxSize) override{
			return 0;
		}
	};
public:
	ProtocolModule(const QString &filename, const QString &config_location, const QString &plugins_location);
	~ProtocolModule();
	operator bool() const{
		return this->ok;
	}
	const std::string &get_protocol_string() const{
		return this->protocol;
	}
	std::unique_ptr<QIODevice> open(const QString &s);
	QStringList enumerate_directory(const QString &s);
};

class CustomProtocolHandler{
	std::unordered_map<std::string, std::unique_ptr<ProtocolModule>> modules;
public:
	CustomProtocolHandler(const QString &config_location);
	std::unique_ptr<QIODevice> open(const QString &s);
	static bool is_url(const QString &);
};

#endif
