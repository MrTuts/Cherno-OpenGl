#pragma once

class IndexBuffer
{
private:
  unsigned int m_RendererID;
  unsigned int m_Count;

public:
  // count means element count
  IndexBuffer(const unsigned int *data, unsigned int count);
  ~IndexBuffer();

  void Bind() const;
  void Unbind() const;

  // The inline keyword suggests replacing a function call with its code to reduce overhead
  // Basically it takes the code and places it at the place where the function is invoked - replaces the function call with the actual code
  inline unsigned int GetCount() const { return m_Count; }
};