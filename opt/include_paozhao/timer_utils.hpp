#ifndef TIMER_UTILS_H
#define TIMER_UTILS_H

#include <chrono>
#include <ostream>

namespace timer_utils
{
	template <class T> class Timer {
		typedef std::chrono::steady_clock steady_clock;
	public:
		explicit Timer()
		{
			Reset();
		}
		void Reset()
		{
			m_start = steady_clock::now();
		}
		T Elapsed() const
		{
			return std::chrono::duration_cast<T>(steady_clock::now() - m_start);
		}

		friend std::ostream& operator<<(std::ostream& out, const Timer& tt)
		{
			return out << tt.Elapsed().count();
		}
	private:
		steady_clock::time_point m_start;
	};
} // namespace timer_utils

#endif // TIMER_UTILS_H