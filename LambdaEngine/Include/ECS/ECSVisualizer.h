#pragma once

namespace LambdaEngine
{
	class JobScheduler;

	class ECSVisualizer
	{
	public:
		ECSVisualizer(JobScheduler* pJobScheduler);
		~ECSVisualizer() = default;

		void Render();

	private:
		JobScheduler* m_pJobScheduler;
	};
}
