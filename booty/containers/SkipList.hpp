#ifndef BOOTY_CONTAINERS_SKIPLIST_HPP
#define BOOTY_CONTAINERS_SKIPLIST_HPP

#include<cassert>
#include<memory>
#include<atomic>
#include<functional>
#include"../detail/Memory.hpp"

namespace booty {

	namespace containers {

		template <
			typename Key,
			class Comparator = std::less<typename Key>,
			class NodeAllocator = detail::SysAllocator<Key>,
			int kMaxHeight = 16,
			int kBranching = 8
		>
			class SkipList {

			static_assert(kMaxHeight >= 16 && kMaxHeight < 64, "kMaxHeight must be in range of [16, 64)");
			static_assert(kMaxHeight & 0x01 == 0, "kMaxHeight must be power of 2");

			struct Node;

			public:
				void Insert(const Key& key);

				void Insert(Key&& key);

				bool Contains(const Key& key);

				class Iterator {
				public:
					explicit Iterator(
						const std::shared_ptr<SkipList<Key, Comparator>>& list);

					bool Valid() const;

					const Key& Key() const;

					void Next();

					void Prev();

					void Seek(const Key& target);

					void SeekToFirst();

					void SeekToLast();

				private:
					const std::shared_ptr<SkipList> list_;
					std::shared_ptr<Node> node_;
				};

				SkipList(const SkipList&) = delete;
				SkipList& operator=(const SkipList&) = delete;

			private:

				inline int getMaxHeight() const {
					return maxHeight_.load(std::memory_order_acquire);
				}

				int randomHeight();

				inline bool isEqual(const Key& lhs, const Key& rhs) const {
					return compare_(lhs, rhs) == 0;
				}

				bool keyIsAfterNode(const Key& key,
					const std::shared_ptr<Node>& node) const;

				std::shared_ptr<Node> findGreaterOrEqual(
					const Key& key, Node** prev) const;

				std::shared_ptr<Node> findLessThan(const Key& key) const;

				std::shared_ptr<Node> findLast() const;

				std::shared_ptr<Node> head_;
				std::atomic<int> maxHeight_;
				const NodeAllocator allocator_;
		};

		template <
			typename Key,
			class Comparator = std::less<typename Key>,
			class NodeAllocator = detail::SysAllocator<Key>,
			int kMaxHeight = 16,
			int kBranching = 8
		>
			struct SkipList<Key, Comparator, NodeAllocator, kMaxHeight, kBranching>::Node {

			explicit Node(const Key& key)
				:key_(key) {}

			static std::shared_ptr<Node> NewNode(const Key& key, int height) {
				assert(height >= 1 && height < kMaxHeight);
				size_t size = sizeof(Node) + height * sizeof(std::atomic<Node*>);
				auto storage = std::allocator_traits<NodeAllocator>::allocate(allocator_, size);
				// do placement new
				return std::make_shared(new (storage) Node(key));
			}

			const Key key_;

			Nood* Next(int n) {
				next_[n].load(std::memory_order_acquire);
			}

			void SetNext(int n, Node* node) {
				next_[n].store(node, std::memory_order_release);
			}

			private:
				std::array<std::atomic<Node*>, kMaxHeight> next_;
		};

	}  // namespace containers

}  // namespace booty

#endif