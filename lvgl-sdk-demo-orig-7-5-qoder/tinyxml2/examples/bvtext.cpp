#include "bvtext.h"
#include <map>
#include <tinyxml2.h>

using namespace tinyxml2;
using namespace std;

class TextDataPrivate {
public:
	TextDataPrivate();
	virtual ~TextDataPrivate();
	void createXML(const std::string& strPath);
	std::vector<std::string> innerSplit(const std::string& strSrc);
	const char* findNode(const std::vector<std::string>& vecStr);
	std::string getValue(const std::string& strNode, const char* value = 0);
	bool existNode(const std::string& strNode);

public:
	XMLDocument* pDoc;
	XMLElement* pRootElement;
};

TextDataPrivate::TextDataPrivate()
{
	pDoc = nullptr;
	pRootElement = nullptr;
}

TextDataPrivate::~TextDataPrivate()
{
	if (pDoc)
		delete pDoc;
}

void TextDataPrivate::createXML(const std::string& strPath)
{
	std::string strRelXmlPath = strPath;
#ifndef WIN32
	strRelXmlPath = "/usr/bv/" + strPath;
#endif // !WIN32

	/* 如果之前加载过，先清理 */
	if (pDoc) {
		delete pDoc;
		pDoc = nullptr;
		pRootElement = nullptr;
	}

	pDoc = new XMLDocument();
	if (!pDoc) {
		printf("<ERROR> failed to allocate XMLDocument\n");
		return;
	}

	XMLError err = pDoc->LoadFile(strRelXmlPath.c_str());
	if (err != XML_SUCCESS) {
		printf("<ERROR> file(%s) : code = %s\n", strRelXmlPath.c_str(), pDoc->ErrorIDToName(err));
		delete pDoc;
		pDoc = nullptr;
		pRootElement = nullptr;
		return;
	}

	pRootElement = pDoc->FirstChildElement("language");
	if (!pRootElement) {
		printf("<ERROR> file(%s) : root element <language> not found\n", strRelXmlPath.c_str());
		delete pDoc;
		pDoc = nullptr;
	}
}

std::vector<std::string> TextDataPrivate::innerSplit(const std::string& strSrc)
{
	std::vector<std::string> elems;
	size_t pos = 0;
	size_t len = strSrc.length();
	std::string delim = "/";

	size_t delim_len = delim.length();
	if (delim_len == 0)
		return elems;

	while (pos < len)
	{
		int find_pos = strSrc.find(delim, pos);
		if (find_pos < 0)
		{
			elems.push_back(strSrc.substr(pos, len - pos));
			break;
		}
		elems.push_back(strSrc.substr(pos, find_pos - pos));
		pos = find_pos + delim_len;
	}
	return elems;
}

const char* TextDataPrivate::findNode(const std::vector<std::string>& vecStr)
{
	if (vecStr.empty())
		return 0;

	if (!pDoc || !pRootElement)
		return 0;

	XMLElement* pElement = pRootElement;

	string strPath;
	for (std::string str : vecStr)
	{
		strPath += str;
		strPath += "/";
		pElement = pElement->FirstChildElement(str.c_str());
		if (!pElement)
			return 0;
	}

	if (pElement)
	{
		return pElement->Attribute("name");
	}
	return 0;
}

std::string TextDataPrivate::getValue(const std::string& strNode, const char* value /*= 0*/)
{
	vector<string> vecStr = innerSplit(strNode);
	if (vecStr.empty())
	{
		if (value)
			return value;

		return "";
	}

	const char* pcBuf = findNode(vecStr);
	if (!pcBuf)
	{
		if (value)
			return value;

		printf("<ERROR> : (%s) node and value is empty \n", strNode.c_str());
		return "";
	}
	return pcBuf;
}

bool TextDataPrivate::existNode(const std::string& strNode)
{
	if (!pRootElement)
		return false;

	tinyxml2::XMLElement* child = pRootElement->FirstChildElement(strNode.c_str());
	if (child)
		return true;
	return false;
}

BvText::BvText()
{
	d_ptr = new TextDataPrivate;
}

BvText& BvText::instance()
{
	static BvText _instance;
	return _instance;
}

void BvText::createXML(const std::string& strPath)
{
	d_ptr->createXML(strPath);
}

std::string BvText::getValue(const std::string& strNode, const char* value /*= 0*/)
{
	return d_ptr->getValue(strNode, value);
}
