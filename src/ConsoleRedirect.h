#ifndef CONSOLEREDIRECT_H
#define CONSOLEREDIRECT_H

#include <streambuf>
#include <string>
#include <windows.h>

// A custom streambuf that appends text to a multiline edit control
class EditStreamBuf : public std::streambuf {
public:
    explicit EditStreamBuf(HWND hEdit);
    ~EditStreamBuf();

protected:
    virtual int_type overflow(int_type ch) override;  // Write char
    virtual int sync() override;                      // Flush

private:
    void flushBuffer();
    HWND m_hEdit;
    std::string m_buffer;
};

#endif // CONSOLEREDIRECT_H
