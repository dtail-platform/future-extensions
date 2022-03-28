// Copyright 2020 Splash Damage, Ltd. - All Rights Reserved.

#include <CoreMinimal.h>
#include <FutureExtensions.h>

#include "Helpers/TestHelpers.h"


#if WITH_DEV_AUTOMATION_TESTS

/************************************************************************/
/* FUTURE BASIC SPEC                                                    */
/************************************************************************/

class FFutureTestSpec_ExecutionPolicy : public FFutureTestSpec
{
	GENERATE_SPEC(FFutureTestSpec_ExecutionPolicy, "FutureExtensions.ExecutionPolicy",
		EAutomationTestFlags::ProductFilter |
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ServerContext
	);

	FFutureTestSpec_ExecutionPolicy() : FFutureTestSpec()
	{
		DefaultTimeout = FTimespan::FromSeconds(0.2);
	}
};


void FFutureTestSpec_ExecutionPolicy::Define()
{
	LatentIt("Can run in a different thread", [this](const auto& Done)
	{
		FE::Async([]()
		{
			return FE::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
		}, FE::FExpectedFutureOptionsBuilder()
			.SetDesiredExecutionThread(ENamedThreads::ActualRenderingThread)
			.Build())
		.Then([this, Done](FE::TExpected<ENamedThreads::Type> Expected)
		{
			TestTrue("Async function is completed", Expected.IsCompleted());
			TestEqual("Execution thread", *Expected, ENamedThreads::ActualRenderingThread);
			Done.Execute();
		});
	});

	LatentIt("Can run in the current thread", [this](const auto& Done)
	{
		auto CurrentThread = FTaskGraphInterface::Get().GetCurrentThreadIfKnown();

		FE::Async([]()
		{
			return FE::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
		}, FE::FExpectedFutureOptionsBuilder()
			.SetExecutionPolicy(FE::EExpectedFutureExecutionPolicy::Current)
			.Build())
		.Then([this, Done, CurrentThread](FE::TExpected<ENamedThreads::Type> Expected)
		{
			TestTrue("Async function is completed", Expected.IsCompleted());
			TestEqual("Execution thread", *Expected, CurrentThread);
			Done.Execute();
		});
	});

	LatentIt("Can run in game thread", [this](const auto& Done)
	{
		auto CurrentThread = FTaskGraphInterface::Get().GetCurrentThreadIfKnown();

		FE::Async([]()
		{
			return FE::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
		}, FE::FExpectedFutureOptionsBuilder()
			.SetExecutionPolicy(FE::EExpectedFutureExecutionPolicy::NamedThread)
			.SetDesiredExecutionThread(ENamedThreads::GameThread)
			.Build())
		.Then([this, Done, CurrentThread](FE::TExpected<ENamedThreads::Type> Expected)
		{
			TestTrue("Async function is completed", Expected.IsCompleted());
			TestEqual("Execution thread", *Expected, ENamedThreads::GameThread);
			Done.Execute();
		});
	});

	LatentIt("Can use previous thread in Then", [this](const auto& Done)
	{
		FE::Async([]()
		{
			return FE::MakeReadyExpected();
		}, FE::FExpectedFutureOptionsBuilder()
			.SetExecutionPolicy(FE::EExpectedFutureExecutionPolicy::NamedThread)
			.SetDesiredExecutionThread(ENamedThreads::GameThread)
			.Build())
		.Then([](FE::TExpected<void> Expected)
		{
			return FE::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
		}, FE::FExpectedFutureOptionsBuilder()
			.SetExecutionPolicy(FE::EExpectedFutureExecutionPolicy::Inline)
			.Build())
		.Then([this, Done](FE::TExpected<ENamedThreads::Type> Expected)
		{
			TestTrue("Async function is completed", Expected.IsCompleted());
			TestEqual("Execution thread", *Expected, ENamedThreads::GameThread);
			Done.Execute();
		});
	});

	LatentIt("Can inline thread in next Then", [this](const auto& Done)
	{
		FE::Async([]()
		{
			return FE::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
		}, FE::FExpectedFutureOptionsBuilder()
			.SetExecutionPolicy(FE::EExpectedFutureExecutionPolicy::ThreadPool)
			.Build())
		.Then([this, Done](FE::TExpected<ENamedThreads::Type> Expected)
		{
			TestTrue("Async function is completed", Expected.IsCompleted());
			TestTrue("Executed on the thread pool", (*Expected & ENamedThreads::ThreadIndexMask) == ENamedThreads::AnyThread);
			Done.Execute();
		});
	});

	LatentIt("Can schedule Then on a worker thread", [this](const auto& Done)
	{
		FE::Async([]()
		{
			return FE::MakeReadyExpected();
		})
		.Then([](FE::TExpected<void> Expected)
		{
			return FE::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
		}, FE::FExpectedFutureOptionsBuilder()
			.SetExecutionPolicy(FE::EExpectedFutureExecutionPolicy::ThreadPool)
			.Build())
		.Then([this, Done](FE::TExpected<ENamedThreads::Type> Expected)
		{
			TestTrue("Async function is completed", Expected.IsCompleted());
			TestTrue("Executed on the thread pool", (*Expected & ENamedThreads::ThreadIndexMask) == ENamedThreads::AnyThread);
			Done.Execute();
		});
	});
}

#endif //WITH_DEV_AUTOMATION_TESTS
