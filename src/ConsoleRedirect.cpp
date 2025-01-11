#include "ConsoleRedirect.h"

EditStreamBuf::EditStreamBuf(HWND hEdit)
    : m_hEdit(hEdit)
{
    m_buffer.reserve(256);
}

EditStreamBuf::~EditStreamBuf() {
    sync();
}

std::streambuf::int_type EditStreamBuf::overflow(int_type ch) {
    if (ch != traits_type::eof()) {
        m_buffer.push_back((char)ch);
        // If newline or big buffer, flush
        if (ch == '\n' || m_buffer.size() > 200) {
            flushBuffer();
        }
    }
    return ch;
}

int EditStreamBuf::sync() {
    flushBuffer();
    return 0;
}

void EditStreamBuf::flushBuffer() {
    if (!m_buffer.empty()) {
        // Convert to wide
        std::wstring wtext(m_buffer.begin(), m_buffer.end());

        // Append to edit
        int len = GetWindowTextLengthW(m_hEdit);
        SendMessageW(m_hEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
        SendMessageW(m_hEdit, EM_REPLACESEL, FALSE, (LPARAM)wtext.c_str());

        m_buffer.clear();
    }
}
