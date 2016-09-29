#include "codec.h"

using namespace muduo;
using namespace muduo::net;
using namespace pubsub;

ParseResult pubsub::parseMessage(Buffer* buf,
	string* cmd,
	string* topic,
	string* content)
{
	ParseResult result = kError;
	const char* crlf = buf->findCRLF(); // ������Ҫ�ҵ���β��
	if (crlf) // �����β���Ǵ��ڵ�
	{
		const char* space = std::find(buf->peek(), crlf, ' '); // �ҵ��ո�
		if (space != crlf)
		{
			cmd->assign(buf->peek(), space); // �������ҵ�����
			topic->assign(space + 1, crlf); // �ҵ�topic
			if (*cmd == "pub") // pub��ʾ��topic������Ϣ
			{
				const char* start = crlf + 2;
				crlf = buf->findCRLF(start); 
				if (crlf)
				{
					content->assign(start, crlf); // �õ�content
					buf->retrieveUntil(crlf + 2); // Ȼ��������Щ����
					result = kSuccess;
				}
				else
				{
					result = kContinue;
				}
			} // if *cmd == "pub"
			else // ������sub����unsub
			{
				buf->retrieveUntil(crlf + 2);
				result = kSuccess;
			} // if space != crlf
		}
		else // ��\r\n��ͷ
		{
			result = kError;
		}
	}
	else// û���ҵ���β��
	{
		result = kContinue;
	}
	return result;
}

