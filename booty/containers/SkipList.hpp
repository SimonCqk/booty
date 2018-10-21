#ifndef BOOTY_CONTAINERS_SKIPLIST_HPP
#define BOOTY_CONTAINERS_SKIPLIST_HPP

#include <memory>
#include<atomic>
#include<functional>

namespace booty {

	namespace containers {

		template <
			typename Key,
			class Comparator = std::less<typename Key>,
			int kMaxHeight = 16,
			int kBranching = 8
		>
			class SkipList {

			static_assert(kMaxHeight >= 16 && kMaxHeight < 64, "kMaxHeight must be in range if [16, 64)");
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
				std::shared_ptr<Node> head_;
				std::atomic<int> maxHeight_;

				inline int getMaxHeight() const {
					return maxHeight_.load(std::memory_order_acquire);
				}

				std::shared_ptr<Node> newNode(const Key& key, int height);
				int randomHeight();

				inline bool isEqual(const Key& lhs, const Key& rhs) const {
					return compare_(lhs, rhs) == 0;
				}

				bool keyIsAfterNode(const Key& key,
					const std::shared_ptr<Node>& node) const;

				std::shared_ptr<Node> findGreaterOrEqual(
					const Key& key, const std::shared_ptr<Node*>& prev) const;

				std::shared_ptr<Node> findLessThan(const Key& key) const;

				std::shared_ptr<Node> findLast() const;
		};
	}  // namespace containers

}  // namespace booty

#endif