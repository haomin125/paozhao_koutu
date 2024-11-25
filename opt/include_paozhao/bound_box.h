#ifndef BOUNDING_BOX_H
#define BOUNDING_BOX_H

#include <iostream>

class BoundingBox
{
public:
	BoundingBox();
	BoundingBox(const unsigned int bottom, const unsigned int top, const unsigned int left, 
				const unsigned int right);
	virtual ~BoundingBox() {}

	unsigned int bottom() { return m_iBottom; }
	void bottom(const unsigned int bottom) { m_iBottom = bottom; }
	unsigned int top() { return m_iTop; }
	void top(const unsigned int top) { m_iTop = top; }
	unsigned int left() { return m_iLeft; }
	void left(const unsigned int left) { m_iLeft = left; }
	unsigned int right() { return m_iRight; }
	void right(const unsigned int right) { m_iRight = right; }

	friend std::ostream& operator<<(std::ostream& os, const BoundingBox &box) {
		os << box.m_iTop << ", " << box.m_iBottom << ", ";
		os << box.m_iLeft << ", " << box.m_iRight;
		return os;
	};
private:
	unsigned int m_iBottom;
	unsigned int m_iTop;
	unsigned int m_iLeft;
	unsigned int m_iRight;
};

#endif // BOUNDIND_BOX_H