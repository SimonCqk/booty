/*
 * This is a derivative snippet of Facebook::folly, under Apache Lisence.
 * Indention:
 * - Recurrent the design idea of seniors and rewrite some details to adapt
 *   personal considerations as components of booty.
 *
 * @Simoncqk - 2018.05.02
 *
 */
#ifndef BOOTY_CONCURRENCY_UNBOUNDEDQUEUE_H
#define BOOTY_CONCURRENCY_UNBOUNDEDQUEUE_H

#include<atomic>
#include<chrono>
#include<new>

namespace booty {

	namespace concurrency {
		/// UnboundedQueue supports a variety of options for unbounded
		/// dynamically expanding an shrinking queues, including variations of:
		/// - Single vs. multiple producers
		/// - Single vs. multiple consumers
		/// - Blocking vs. spin-waiting
		/// - Non-waiting, timed, and waiting consumer operations.
		/// Producer operations never wait or fail (unless out-of-memory).
		///
		/// Template parameters:
		/// - T: element type
		/// - SingleProducer: true if there can be only one producer at a
		///   time.
		/// - SingleConsumer: true if there can be only one consumer at a
		///   time.
		/// - MayBlock: true if consumers may block, false if they only
		///   spin. A performance tuning parameter.
		/// - LgSegmentSize (default 8): Log base 2 of number of elements per
		///   segment. A performance tuning parameter. See below.
		/// - LgAlign (default 7): Log base 2 of alignment directive; can be
		///   used to balance scalability (avoidance of false sharing) with
		///   memory efficiency.
		///
		/// When to use UnboundedQueue:
		/// - If a small bound may lead to deadlock or performance degradation
		///   under bursty patterns.
		/// - If there is no risk of the queue growing too much.
		///
		/// When not to use UnboundedQueue:
		/// - If there is risk of the queue growing too much and a large bound
		///   is acceptable, then use DynamicBoundedQueue.
		/// - If the queue must not allocate on enqueue or it must have a
		///   small bound, then use fixed-size MPMCQueue or (if non-blocking
		///   SPSC) ProducerConsumerQueue.

		/// Memory Usage:
		/// - An empty queue contains one segment. A nonempty queue contains
		///   one or two more segment than fits its contents.
		/// - Removed segments are not reclaimed until there are no threads,
		///   producers or consumers, have references to them or their
		///   predecessors. That is, a lagging thread may delay the reclamation
		///   of a chain of removed segments.
		/// - The template parameter LgAlign can be used to reduce memory usage
		///   at the cost of increased chance of false sharing.
		///
		/// Performance considerations:
		/// - All operations take constant time, excluding the costs of
		///   allocation, reclamation, interference from other threads, and
		///   waiting for actions by other threads.
		/// - In general, using the single producer and or single consumer
		///   variants yield better performance than the MP and MC
		///   alternatives.
		/// - SPSC without blocking is the fastest configuration. It doesn't
		///   include any read-modify-write atomic operations, full fences, or
		///   system calls in the critical path.
		/// - MP adds a fetch_add to the critical path of each producer operation.
		/// - MC adds a fetch_add or compare_exchange to the critical path of
		///   each consumer operation.
		/// - The possibility of consumers blocking, even if they never do,
		///   adds a compare_exchange to the critical path of each producer
		///   operation.
		/// - MPMC, SPMC, MPSC require the use of a deferred reclamation
		///   mechanism to guarantee that segments removed from the linked
		///   list, i.e., unreachable from the head pointer, are reclaimed
		///   only after they are no longer needed by any lagging producers or
		///   consumers.
		/// - The overheads of segment allocation and reclamation are intended
		///   to be mostly out of the critical path of the queue's throughput.
		/// - If the template parameter LgSegmentSize is changed, it should be
		///   set adequately high to keep the amortized cost of allocation and
		///   reclamation low.
		/// - Another consideration is that the queue is guaranteed to have
		///   enough space for a number of consumers equal to 2^LgSegmentSize
		///   for local blocking. Excess waiting consumers spin.
		/// - It is recommended to measure performance with different variants
		///   when applicable, e.g., UMPMC vs UMPSC. Depending on the use
		///   case, sometimes the variant with the higher sequential overhead
		///   may yield better results due to, for example, more favorable
		///   producer-consumer balance or favorable timing for avoiding
		///   costly blocking.

		template<typename T, bool SingleProducer, bool SingleConsumer,bool MayBlock,
			size_t LogSegmentSize = 8, size_t LogAlign = std::log2(std::hardware_constructive_interference_size),
			template<typename> class Atom = std::atomic>
		class UnboundedQueue {
			using Ticket = uint64_t;

			class Entry;
			class Segment;

			static constexpr bool SPSC = SingleProducer && SingleConsumer;
			static constexpr size_t Stride = SPSC || (LogSegmentSize <= 1) ? 1 : 27;
			static constexpr size_t SegmentSize = 1u << LogSegmentSize;
			static constexpr size_t Align = 1u << LogAlign;

			static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow_destructible.");
			static_assert((Stride & 1) == 1, "Stride must be odd.");
			static_assert(LogSegmentSize < 32, "LogSegmentSize must be < 32.");
			static_assert(LogAlign < 16, "LogAlign must be < 16.");

			struct Consumer {
				Atom<Segment*> head;
				Atom<Ticket> ticket;
			};
			struct Producer {
				Atom<Segment*> tail;
				Atom<Ticket> ticket;
			};

		};


		class Entry {

		};

		class Segment {

		};
	}

}


#endif // !BOOTY_CONCURRENCY_UNBOUNDEDQUEUE_H

