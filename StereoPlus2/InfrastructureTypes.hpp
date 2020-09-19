#pragma once 

#include <stdlib.h>
#include <chrono>
#include <iostream>
#include "TemplateExtensions.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <map>

template<typename T>
int find(const std::vector<T>& source, const T& item) {
	for (size_t i = 0; i < source.size(); i++)
		if (source[i] == item)
			return i;

	return -1;
}
template<typename T>
int find(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	for (size_t i = 0; i < source.size(); i++)
		if (condition(source[i]))
			return i;

	return -1;
}
template<typename T>
std::vector<int> findAll(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	std::vector<int> indices;
	for (size_t i = 0; i < source.size(); i++)
		if (condition(source[i]))
			indices.push_back(i);

	return indices;
}
template<typename T>
std::vector<int> findAllBack(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	std::vector<int> indices;
	for (size_t i = source.size(); i-- != 0; )
		if (condition(source[i]))
			indices.push_back(i);

	return indices;
}
template<typename T>
bool exists(const std::set<T>& source, const T& item) {
	return source.find(item) != source.end();
}
template<typename T>
bool exists(const std::set<const T>& source, const T& item) {
	return source.find(item) != source.end();
}
template<typename K, typename T>
bool exists(const std::set<K>& source, const T& item, std::function<T(const K&)> selector) {
	for (auto o : source)
		if (selector(o) == item)
			return true;

	return false;
}

template<typename T>
bool exists(const std::vector<T>& source, const T& item) {
	return std::find(source.begin(), source.end(), item) != source.end();
}
template<typename T>
bool exists(const std::list<T>& source, const T& item) {
	return std::find(source.begin(), source.end(), item) != source.end();
}
template<typename K, typename V>
bool keyExists(const std::set<K,V>& source, const K& item) {
	return source.find(item) != source.end();
}



class Log {
	template<typename T>
	using isOstreamable = decltype(std::declval<std::ostringstream>() << std::declval<T>());

	template<typename T>
	static constexpr bool isOstreamableT = is_detected_v<isOstreamable, T>;

	std::string contextName = "";

	static void Line(const std::string& message) {
		std::cout << message << std::endl;
	}
	template<typename T, std::enable_if_t<isOstreamableT<T>> * = nullptr>
	static std::string ToString(const T& message) {
		std::ostringstream ss;
		ss << message;
		return ss.str();
	}

public:
	template<typename T>
	static const Log For() {
		Log log;
		log.contextName = typeid(T).name();
		return log;
	}

	template<typename... T>
	void Error(const T&... message) const {
		Error((Log::ToString(message) + ...));
	}
	template<typename... T>
	void Warning(const T&... message) const {
		Warning((Log::ToString(message) + ...));
	}
	template<typename... T>
	void Information(const T&... message) const {
		Information((Log::ToString(message) + ...));
	}


	void Error(const std::string& message) const {
		Line("[Error](" + contextName + ") " + message);
	}
	void Warning(const std::string& message) const {
		Line("[Warning](" + contextName + ") " + message);
	}
	void Information(const std::string& message) const {
		Line("[Information](" + contextName + ") " + message);
	}
};

class Time {
	static std::chrono::steady_clock::time_point* GetBegin() {
		static std::chrono::steady_clock::time_point instance;
		return &instance;
	}
	static size_t* GetDeltaTimeMicroseconds() {
		static size_t instance;
		return &instance;
	}
public:
	static void UpdateFrame() {
		auto end = std::chrono::steady_clock::now();
		*GetDeltaTimeMicroseconds() = std::chrono::duration_cast<std::chrono::microseconds>(end - *GetBegin()).count();
		*GetBegin() = end;
	}
	static float GetFrameRate() {
		return 1 / GetDeltaTime();
	}
	static float GetDeltaTime() {
		return (float)*GetDeltaTimeMicroseconds() / 1e6;
	}
	static size_t GetTime() {
		return std::chrono::time_point_cast<std::chrono::milliseconds>(*GetBegin()).time_since_epoch().count();
	}
};

class Command {
	static std::list<Command*>& GetQueue() {
		static auto queue = std::list<Command*>();
		return queue;
	}
protected:
	bool isReady = false;
	virtual bool Execute() = 0;
public:
	Command() {
		GetQueue().push_back(this);
	}
	static bool ExecuteAll() {
		std::list<Command*> deleteQueue;
		for (auto command : GetQueue())
			if (command->isReady) {
				if (!command->Execute())
					return false;

				deleteQueue.push_back(command);
			}

		for (auto command : deleteQueue) {
			GetQueue().remove(command);
			delete command;
		}

		return true;
	}
};

class FuncCommand : Command {
protected:
	virtual bool Execute() {
		func();

		return true;
	};
public:
	FuncCommand() {
		isReady = true;
	}

	std::function<void()> func;
};

template<typename...T>
class IEvent {
protected:
	std::map<size_t, std::function<void(T...)>> handlers;
public:
	size_t AddHandler(std::function<void(T...)> func) {
		static size_t id = 0;

		(new FuncCommand())->func = [&, id = id, f = func] {
			handlers[id] = f;
		};

		return id++;
	}
	void RemoveHandler(size_t v) {
		(new FuncCommand())->func = [&, v = v] {
			handlers.erase(v);
		};
	}
};

template<typename...T>
class Event : public IEvent<T...> {
public:
	void Invoke(T... vs) {
		for (auto h : IEvent<T...>::handlers)
			h.second(vs...);
	}
};

template<typename T>
class Property {
	template<typename T>
	class Node {
		T value;
		Event<T> changed;
	public:
		const T& Get() const {
			return value;
		}
		T& Get() {
			return value;
		}
		void Set(const T& v) {
			auto old = value;
			value = v;
			if (old != v)
				changed.Invoke(v);
		}
		IEvent<T>& OnChanged() const {
			return *(IEvent<T>*) & changed;
		}
	};
	
	std::shared_ptr<Node<T>> node = std::make_shared<Node<T>>();
public:
	const T& Get() const {
		return node->Get();
	}
	T& Get() {
		return node->Get();
	}
	void Set(const T& v) {
		node->Set(v);
	}
	IEvent<T>& OnChanged() const {
		return node->OnChanged();
	}
	void Bind(const Property<T>& p) {
		p.OnChanged().AddHandler([&](const T& o) { this->Set(o); });
	}
	void BindTwoWay(const Property<T>& p) {
		node = p.node;
	}

	Property<T>& operator=(const T& v) {
		Set(v);
		return *this;
	}
};

#define StaticProperty(type,name) \
static Property<type>& name() {\
	static Property<type> v;\
	return v;\
}
