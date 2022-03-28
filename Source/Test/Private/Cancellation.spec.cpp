// Copyright 2020 Splash Damage, Ltd. - All Rights Reserved.

#include <CoreMinimal.h>
#include <FutureExtensions.h>

#include "Helpers/TestHelpers.h"


#if WITH_DEV_AUTOMATION_TESTS

/************************************************************************/
/* FUTURE CANCELLATION SPEC                                             */
/************************************************************************/

class FFutureTestSpec_Cancellation : public FFutureTestSpec
{
	GENERATE_SPEC(FFutureTestSpec_Cancellation, "FutureExtensions.Cancellation",
		EAutomationTestFlags::ProductFilter |
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ServerContext
	);

	bool bInitFunctionExecuted = false;
	bool bThenExecuted = false;

	FE::SharedCancellationHandleRef CancellationHandle = FE::CreateCancellationHandle();

	FFutureTestSpec_Cancellation() : FFutureTestSpec()
	{
		DefaultTimeout = FTimespan::FromSeconds(1.5);
	}
};


void FFutureTestSpec_Cancellation::Define()
{
	BeforeEach([this]()
	{
		CancellationHandle = FE::CreateCancellationHandle();
		bInitFunctionExecuted = false;
		bThenExecuted = false;
	});

	LatentIt("Can deal with a cancel race condition", [this](const auto& Done)
	{
		FE::TExpectedFuture<int32> Future = FE::Async([]() {
			return FE::MakeReadyExpected(5);
		}, FE::FExpectedFutureOptions(CancellationHandle));

		CancellationHandle->Cancel();

		Future.Then([this, Done](FE::TExpected<int32> Expected)
		{
			//This is weird, but we *expect* cancellation to be a race condition. Cancellation is a best-effort process.
			//It may, or may not, cancel before the value is set. This test is here to make sure that we handle the race condition.
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled() || Expected.IsCompleted());
			Done.Execute();
		});
	});

	LatentIt("Can cancel a Future", [this](const auto& Done)
	{
		FE::TExpectedFuture<int32> Future = FE::Async([]()
		{
			//Force a sleep here to avoid the race condition described above - we want to ensure a cancel.
			FPlatformProcess::Sleep(1.0f);
			return FE::MakeReadyExpected(5);
		}, FE::FExpectedFutureOptions(CancellationHandle));

		CancellationHandle->Cancel();

		Future.Then([this, Done](FE::TExpected<int32> Expected)
		{
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled());
			Done.Execute();
		});
	});

	LatentIt("Can cancel before the Future is set", [this](const auto& Done)
	{
		CancellationHandle->Cancel();

		FE::Async([]()
		{
			return FE::MakeReadyExpected(5);
		}, FE::FExpectedFutureOptions(CancellationHandle))
		.Then([this, Done](FE::TExpected<int32> Expected)
		{
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled());
			Done.Execute();
		});
	});

	LatentIt("Expect Then is called after cancel", [this](const auto& Done)
	{
		FE::TExpectedFuture<void> Future = FE::Async([]()
		{
			//Force a sleep here to avoid the race condition described above - we want to ensure a cancel.
			FPlatformProcess::Sleep(1.0f);
			return FE::MakeReadyExpected(5);
		}, FE::FExpectedFutureOptions(CancellationHandle))
		.Then([this](FE::TExpected<int32> Expected)
		{
			bThenExecuted = true;
			return FE::Convert(Expected);
		});

		CancellationHandle->Cancel();

		Future.Then([this, Done](FE::TExpected<void> Expected)
		{
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled());
			TestTrue("Expected-based continuation was called", bThenExecuted);
			Done.Execute();
		});
	});

	LatentIt("Cancelled Then wont be called", [this](const auto& Done)
	{
		CancellationHandle->Cancel();

		FE::Async([this]()
		{
			bInitFunctionExecuted = true;
			return FE::MakeReadyExpected(5);
		})
		.Then([this](FE::TExpected<int32> Expected)
		{
			bThenExecuted = true;
			return FE::Convert(Expected);
		}, FE::FExpectedFutureOptions(CancellationHandle))
		.Then([this, Done](FE::TExpected<void> Expected)
		{
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled());
			TestTrue("Initial function was called", bInitFunctionExecuted);
			TestFalse("Expected-based continuation was called", bThenExecuted);
			Done.Execute();
		});
	});

	LatentIt("Expect Then is called after cancelled Then", [this](const auto& Done)
	{
		CancellationHandle->Cancel();

		FE::Async([]()
		{
			return FE::MakeReadyExpected(5);
		})
		.Then([this](FE::TExpected<int32> Expected)
		{
			return Expected;
		}, FE::FExpectedFutureOptions(CancellationHandle))
		.Then([](FE::TExpected<int32> Expected)
		{
			//Then expected-based continuation is called with "Cancelled" status
			return FE::MakeReadyExpected<bool>(Expected.IsCancelled());
		})
		.Then([this, Done](FE::TExpected<bool> Expected)
		{
			TestTrue("Expected is completed", Expected.IsCompleted());
			TestTrue("Then received was 'cancel' state", *Expected);
			Done.Execute();
		});
	});

	LatentIt("Then is not called with raw value after Cancellation", [this](const auto& Done)
	{
		CancellationHandle->Cancel();

		FE::Async([]()
		{
			return FE::MakeReadyExpected(5);
		})
		.Then([this](FE::TExpected<int32> Expected)
		{
			return Expected;
		}, FE::FExpectedFutureOptions(CancellationHandle))
		.Then([this](int32 Result)
		{
			bThenExecuted = true;
			return true;
		})
		.Then([this, Done](FE::TExpected<bool> Expected)
		{
			TestTrue("Expected is completed", Expected.IsCancelled());
			TestFalse("Then with raw value executed", bThenExecuted);
			Done.Execute();
		});
	});
}

#endif //WITH_DEV_AUTOMATION_TESTS
