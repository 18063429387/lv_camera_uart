#ifndef BVTEXT_H
#define BVTEXT_H

#include <string>
#include <vector>

class TextDataPrivate;

class BvText;
#define xmlText BvText::instance()

class BvText
{
protected:
	BvText();
	BvText(const BvText&) = delete;
	BvText& operator=(const BvText&) = delete;

public:
	static BvText& instance();
	void createXML(const std::string& strPath);
	std::string getValue(const std::string& strNode, const char* value = 0);

private:
	TextDataPrivate* d_ptr;
};

#endif // BVTEXT_H
