/*
 * Copyright 2015 Samsung Austin Semiconductor, LLC.
 */

/*!
 * \file    bt9_reader.h
 * \brief   This is a reader library for Branch Trace version 9 (BT9) format.
 */

#ifndef __BT9_READER_H__
#define __BT9_READER_H__

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <limits>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <vector>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include "bt9.h"

namespace bt9 {

/*!
 * \class BT9ReaderHeader
 * \brief It inherits from the BasicHeader, and is used by the BT9 reader library
 */
class BT9ReaderHeader : public BasicHeader {
    public:
        using Dictionary = std::unordered_map<std::string, std::string>;

        BT9ReaderHeader() = default;

        BT9ReaderHeader(const BT9ReaderHeader &) = default;

        /// Get the value(string) of user-defined key-value pairs
        bool getFieldValueStr(const std::string &name, std::string &value) const {
            auto it = unclassified_fields_.find(name);
            if (it != unclassified_fields_.end()) {
                value = it->second;
                return true;
            }
            return false;
        }

        friend class BT9Reader;

    protected:
        Dictionary unclassified_fields_;
};

/*!
 * \class BT9ReaderNodeRecord
 * \brief It inherits from the BasicNodeRecord, and is used by the BT9 reader library
 */
class BT9ReaderNodeRecord : public BasicNodeRecord {
    public:
        using Dictionary = std::unordered_map<std::string, std::string>;

        BT9ReaderNodeRecord() = default;

        BT9ReaderNodeRecord(const BT9ReaderNodeRecord &) = default;

        /// Get dynamically observed branch target counts for this branch
        uint32_t brObservedTargetCnt() const { return br_tgt_cnt_; }

        /// Get the value(string) of user-defined key-value pairs
        bool getFieldValueStr(const std::string &name, std::string &value) const {
            auto it = unclassified_fields_.find(name);
            if (it != unclassified_fields_.end()) {
                value = it->second;
                return true;
            }
            return false;
        }

        /*!
         * \brief Print optional fields of branch node record
         *
         * This overrides the counterpart in its base class, customized for BT9Reader
         */
        void printOptionalFields(std::ostream &os) const override {
            std::ios_base::fmtflags saveflags = os.flags();
            std::streamsize prec = os.precision();
            std::streamsize width = os.width();


            // Bypass for dummy source/sink node
            if (opcode_size_ == 0) {
                return;
            }

            // Sanity Check
            if (br_class_.directness == BrClass::Directness::DIRECT &&
                br_behavior_.indirectness == BrBehavior::Indirectness::INDIRECT) {

                std::cerr << "BrClass:" << BrClass::Directness::DIRECT
                          << " can never lead to BrBehavior: " << BrBehavior::Indirectness::INDIRECT
                          << std::endl;

                exit(-1);
            }

            // Print pre-defined key-value pairs
            BasicNodeRecord::printOptionalFields(os);

            os << std::setw(2) << " "
               << "tgt_cnt: " << std::setw(4) << std::dec << br_tgt_cnt_ << std::setw(2) << " "
               << "mnemonic: \"" << mnemonic_ << "\"";

            // Print user-defined key-value pairs
            for (const auto &field : unclassified_fields_) {
                os << field.first << " " << field.second;
            }


            os.flags(saveflags);
            os.precision(prec);
            os.width(width);
        }

        friend class BT9Reader;

    protected:
        uint32_t br_tgt_cnt_ = 0;
        Dictionary unclassified_fields_;
};

/*!
 * \class BT9ReaderEdgeRecord
 * \brief This inherits from the BasicEdgeRecord, and is used by the BT9 reader library
 */
class BT9ReaderEdgeRecord : public BasicEdgeRecord {
    public:
        BT9ReaderEdgeRecord() = default;

        BT9ReaderEdgeRecord(const BT9ReaderEdgeRecord &) = default;

        using Dictionary = std::unordered_map<std::string, std::string>;

        /// Get the value(string) of user-defined key-value pairs
        bool getFieldValueStr(const std::string &name, std::string &value) const {
            auto it = unclassified_fields_.find(name);
            if (it != unclassified_fields_.end()) {
                value = it->second;
                return true;
            }
            return false;
        }

        /*!
         * \brief Print optional fields of branch edge record
         *
         * This overrides the counterpart in its base class, customized for BT9Reader
         */
        void printOptionalFields(std::ostream &os) const override {
            BasicEdgeRecord::printOptionalFields(os);

            for (const auto &field : unclassified_fields_) {
                os << field.first << " " << field.second;
            }
        }

        friend class BT9Reader;

    protected:
        Dictionary unclassified_fields_;
};

/*!
 * \class BT9BranchInstance
 * \brief The dereferenced data type returned by BT9Reader BranchInstanceIterator
 * \note It contains const raw pointers to edge record, src/dest branch node records
 */
class BT9BranchInstance {
    public:
        /// Default constructor
        BT9BranchInstance() = default;

        /*!
         * \brief Constructor
         * \param src_node_ptr Raw pointer to source node record
         * \param dest_node_ptr Raw pointer to destination node record
         * \param edge_ptr Raw pointer to edge record
         */
        BT9BranchInstance(const BT9ReaderNodeRecord *src_node_ptr,
                          const BT9ReaderNodeRecord *dest_node_ptr,
                          const BT9ReaderEdgeRecord *edge_ptr) :
                src_node_rec_ptr_(src_node_ptr),
                dest_node_rec_ptr_(dest_node_ptr),
                edge_rec_ptr_(edge_ptr),
                valid_(true) {}

        /// Copy constructor
        BT9BranchInstance(const BT9BranchInstance &rhs) :
                src_node_rec_ptr_(rhs.src_node_rec_ptr_),
                dest_node_rec_ptr_(rhs.dest_node_rec_ptr_),
                edge_rec_ptr_(rhs.edge_rec_ptr_),
                valid_(rhs.valid_) {}

        /// Indicate if this BT9BranchInstance is valid
        bool isValid() const { return valid_; }

        /// Get the raw pointer to source node record
        const BT9ReaderNodeRecord *getSrcNode() const {
            assert(src_node_rec_ptr_ != nullptr);
            return src_node_rec_ptr_;
        }

        /// Get the raw pointer to destination node record
        const BT9ReaderNodeRecord *getDestNode() const {
            assert(dest_node_rec_ptr_ != nullptr);
            return dest_node_rec_ptr_;
        }

        /// Get the raw pointer to edge record
        const BT9ReaderEdgeRecord *getEdge() const {
            assert(edge_rec_ptr_ != nullptr);
            return edge_rec_ptr_;
        }

        friend class BT9Reader;

    private:
        /// Invalidate BT9BranchInstance
        void invalidate_() {
            valid_ = false;
            src_node_rec_ptr_ = nullptr;
            dest_node_rec_ptr_ = nullptr;
            edge_rec_ptr_ = nullptr;
        }

        /// Update BT9BranchInstance
        void update_(const BT9ReaderNodeRecord *src_node_ptr,
                     const BT9ReaderNodeRecord *dest_node_ptr,
                     const BT9ReaderEdgeRecord *edge_ptr) {
            src_node_rec_ptr_ = src_node_ptr;
            dest_node_rec_ptr_ = dest_node_ptr;
            edge_rec_ptr_ = edge_ptr;
            valid_ = true;
        }

        const BT9ReaderNodeRecord *src_node_rec_ptr_ = nullptr;
        const BT9ReaderNodeRecord *dest_node_rec_ptr_ = nullptr;
        const BT9ReaderEdgeRecord *edge_rec_ptr_ = nullptr;
        bool valid_ = false;
};


/*!
 * \class BT9Reader
 * \brief This is the reader library for Branch Trace Version 9 (BT9) file.
 */
class BT9Reader {
    public:
        /*!
         * \brief Constructor
         * \param filename BT9 trace file name
         * \param buffer_size BT9 edge (i.e. branch instance) sequence list access window size
         */
        BT9Reader(const std::string &name,
                  const uint64_t &buffer_size = 1024) :
                node_table(this),
                edge_table(this),
                tracefile_name_(name),
                fpstream_(openBT9TraceFile_(tracefile_name_), boost::iostreams::close_handle),
                pinfile_(&fpstream_),
                buffer_(buffer_size) {
            readBT9Header_();
            readBT9NodeTable_();
            readBT9EdgeTable_();
            initBT9EdgeSeqListAccessWindow_();
        }

        BT9Reader() = delete;

        BT9Reader(const BT9Reader &) = delete;

        BT9Reader(BT9Reader &&) = delete;

        BT9Reader &operator=(const BT9Reader &) = delete;

        ~BT9Reader() {
        }

        class NodeTableIterator;

        friend class NodeTableIterator;

        /*!
         * \class NodeTableIterator
         * \brief This is the iterator for internal branch node table.
         * \note This is supposed to be a random-access iterator
         */
        class NodeTableIterator {
            public:
                NodeTableIterator() = delete;

                /*!
                 * \brief Constructor
                 * \param rd Pointer to its associated BT9 reader
                 * \param idx Node record id
                 */
                NodeTableIterator(const BT9Reader *rd, const uint32_t &idx) :
                        bt9_reader_(rd),
                        index_(idx) {
                    assert(bt9_reader_ != nullptr);
                }

                /// Copy constructor
                NodeTableIterator(const NodeTableIterator &rhs) :
                        bt9_reader_(rhs.bt9_reader_),
                        index_(rhs.index_) {
                    assert(bt9_reader_ != nullptr);
                }

                /// Copy assignment operator
                NodeTableIterator &operator=(const NodeTableIterator &rhs) {
                    bt9_reader_ = rhs.bt9_reader_;
                    index_ = rhs.index_;

                    assert(bt9_reader_ != nullptr);
                }

                /// Pre-increment operator
                NodeTableIterator &operator++() {
                    ++index_;
                    return *this;
                }

                /// Post-increment operator
                NodeTableIterator operator++(int) {
                    NodeTableIterator it(*this);
                    ++index_;
                    return it;
                }

                /// Equal operator
                bool operator==(const NodeTableIterator &rhs) const {
                    return ((bt9_reader_ == rhs.bt9_reader_) && (index_ == rhs.index_));
                }

                /// Not-equal operator
                bool operator!=(const NodeTableIterator &rhs) const {
                    return !(this->operator==(rhs));
                }

                /*!
                 * \brief Dereference operator
                 * \note Bound-checking is supported. If the itertor internal index is invalid,
                 *       the std::invalid_argument exception is thrown.
                 */
                BT9ReaderNodeRecord &operator*() {
                    if (bt9_reader_->isValidNodeIndex_(index_)) {
                        return *(bt9_reader_->node_order_vector_[index_]);
                    } else {
                        throw std::invalid_argument("Invalid Node Index!\n");
                    }
                }

                /*!
                 * \brief Dereference operator
                 * \note Bound-checking is supported. If the itertor internal index is invalid,
                 *       the std::invalid_argument exception is thrown.
                 */
                BT9ReaderNodeRecord *operator->() {
                    if (bt9_reader_->isValidNodeIndex_(index_)) {
                        return bt9_reader_->node_order_vector_[index_];
                    } else {
                        throw std::invalid_argument("Invalid Node Index!\n");
                    }
                }

                NodeTableIterator operator+(uint32_t offset) const {
                    NodeTableIterator it(bt9_reader_, index_ + offset);
                    return it;
                }

                NodeTableIterator operator-(uint32_t offset) const {
                    NodeTableIterator it(bt9_reader_, index_ - offset);
                    return it;
                }

                int64_t operator-(const NodeTableIterator &rhs) const {
                    return (index_ - rhs.index_);
                }

                bool operator<(const NodeTableIterator &rhs) const {
                    return (index_ < rhs.index_);
                }

                bool operator>(const NodeTableIterator &rhs) const {
                    return (index_ > rhs.index_);
                }

                bool operator<=(const NodeTableIterator &rhs) const {
                    return !this->operator>(rhs);
                }

                bool operator>=(const NodeTableIterator &rhs) const {
                    return !this->operator<(rhs);
                }

                NodeTableIterator operator+=(uint32_t offset) {
                    index_ += offset;
                    return *this;
                }

                NodeTableIterator operator-=(uint32_t offset) {
                    index_ -= offset;
                    return *this;
                }

                BT9ReaderNodeRecord &operator[](uint32_t idx) {
                    if (bt9_reader_->isValidNodeIndex_(idx)) {
                        return *(bt9_reader_->node_order_vector_[idx]);
                    } else {
                        throw std::invalid_argument("Invalid Node Index!\n");
                    }
                }

                const BT9ReaderNodeRecord &operator[](uint32_t idx) const {
                    if (bt9_reader_->isValidNodeIndex_(idx)) {
                        return *(bt9_reader_->node_order_vector_[idx]);
                    } else {
                        throw std::invalid_argument("Invalid Node Index!\n");
                    }
                }

            private:
                const BT9Reader *bt9_reader_ = nullptr;
                uint32_t index_ = 0;
        };

        /*!
         * \class NodeTable
         * \brief This is a wrapper class of internal node table
         * \note It provides the iterator interface that is accessible to the user
         */
        class NodeTable {
            public:
                NodeTableIterator begin() const { return bt9_reader_->nodeTableBegin_(); }

                NodeTableIterator end() const { return bt9_reader_->nodeTableEnd_(); }

                friend std::ostream &operator<<(std::ostream &, const NodeTable &);

                friend BT9Reader;

            private:
                void print_(std::ostream &os) const {
                    assert(bt9_reader_ != nullptr);

                    os << "BT9_NODES\n";
                    os << "#NODE  "
                       << std::setw(4) << " id  "
                       << std::setw(20) << " virtual_address "
                       << std::setw(20) << " physical_address "
                       << std::setw(16) << " opcode "
                       << std::setw(4) << " size "
                       << '\n';

                    for (const auto &node : bt9_reader_->node_table) {
                        os << "NODE: " << node << '\n';
                    }
                }

                NodeTable(const BT9Reader *rd) : bt9_reader_(rd) {};

                NodeTable(const NodeTable &) = delete;

                NodeTable(NodeTable &&) = delete;

                NodeTable &operator=(const NodeTable &) = delete;

                const BT9Reader *bt9_reader_ = nullptr;
        };

        class EdgeTableIterator;

        friend class EdgeTableIterator;

        /*!
         * \class EdgeTableIterator
         * \brief This is the iterator for internal branch edge table.
         * \note This is supposed to be a random-access iterator
         */
        class EdgeTableIterator {
            public:
                EdgeTableIterator() = delete;

                /*!
                 * \brief Constructor
                 * \param rd Pointer to its associated BT9 reader
                 * \param idx Edge record id
                 */
                EdgeTableIterator(const BT9Reader *rd, const uint32_t &idx) :
                        bt9_reader_(rd),
                        index_(idx) {
                    assert(bt9_reader_ != nullptr);
                }

                /// Copy constructor
                EdgeTableIterator(const EdgeTableIterator &rhs) :
                        bt9_reader_(rhs.bt9_reader_),
                        index_(rhs.index_) {
                    assert(bt9_reader_ != nullptr);
                }

                /// Copy assignment operator
                EdgeTableIterator &operator=(const EdgeTableIterator &rhs) {
                    bt9_reader_ = rhs.bt9_reader_;
                    index_ = rhs.index_;

                    assert(bt9_reader_ != nullptr);
                }

                /// Pre-increment operator
                EdgeTableIterator &operator++() {
                    ++index_;
                    return *this;
                }

                /// Post-increment operator
                EdgeTableIterator operator++(int) {
                    EdgeTableIterator it(*this);
                    ++index_;
                    return it;
                }

                /// Equal operator
                bool operator==(const EdgeTableIterator &rhs) const {
                    return ((bt9_reader_ == rhs.bt9_reader_) && (index_ == rhs.index_));
                }

                /// Not-equal operator
                bool operator!=(const EdgeTableIterator &rhs) const {
                    return !(this->operator==(rhs));
                }

                /*!
                 * \brief Dereference operator
                 * \note Bound-checking is supported. If the itertor internal index is invalid,
                 *       the std::invalid_argument exception is thrown.
                 */
                BT9ReaderEdgeRecord &operator*() {
                    if (bt9_reader_->isValidEdgeIndex_(index_)) {
                        return *(bt9_reader_->edge_order_vector_[index_]);
                    } else {
                        throw std::invalid_argument("Invalid Edge Index!\n");
                    }
                }

                /*!
                 * \brief Dereference operator
                 * \note Bound-checking is supported. If the itertor internal index is invalid,
                 *       the std::invalid_argument exception is thrown.
                 */
                BT9ReaderEdgeRecord *operator->() {
                    if (bt9_reader_->isValidEdgeIndex_(index_)) {
                        return bt9_reader_->edge_order_vector_[index_];
                    } else {
                        throw std::invalid_argument("Invalid Edge Index!\n");
                    }
                }

                EdgeTableIterator operator+(uint32_t offset) const {
                    EdgeTableIterator it(bt9_reader_, index_ + offset);
                    return it;
                }

                EdgeTableIterator operator-(uint32_t offset) const {
                    EdgeTableIterator it(bt9_reader_, index_ - offset);
                    return it;
                }

                int64_t operator-(const EdgeTableIterator &rhs) const {
                    return (index_ - rhs.index_);
                }

                bool operator<(const EdgeTableIterator &rhs) const {
                    return (index_ < rhs.index_);
                }

                bool operator>(const EdgeTableIterator &rhs) const {
                    return (index_ > rhs.index_);
                }

                bool operator<=(const EdgeTableIterator &rhs) const {
                    return !this->operator>(rhs);
                }

                bool operator>=(const EdgeTableIterator &rhs) const {
                    return !this->operator<(rhs);
                }

                EdgeTableIterator operator+=(uint32_t offset) {
                    index_ += offset;
                    return *this;
                }

                EdgeTableIterator operator-=(uint32_t offset) {
                    index_ -= offset;
                    return *this;
                }

                BT9ReaderEdgeRecord &operator[](uint32_t idx) {
                    if (bt9_reader_->isValidEdgeIndex_(idx)) {
                        return *(bt9_reader_->edge_order_vector_[idx]);
                    } else {
                        throw std::invalid_argument("Invalid Edge Index!\n");
                    }
                }

                const BT9ReaderEdgeRecord &operator[](uint32_t idx) const {
                    if (bt9_reader_->isValidEdgeIndex_(idx)) {
                        return *(bt9_reader_->edge_order_vector_[idx]);
                    } else {
                        throw std::invalid_argument("Invalid Edge Index!\n");
                    }
                }

            private:
                const BT9Reader *bt9_reader_ = nullptr;
                uint32_t index_ = 0;
        };

        /*!
         * \class EdgeTable
         * \brief This is a wrapper class of internal edge table
         * \note It provides the iterator interface that is accessible to the user
         */
        class EdgeTable {
            public:
                EdgeTableIterator begin() const { return bt9_reader_->edgeTableBegin_(); }

                EdgeTableIterator end() const { return bt9_reader_->edgeTableEnd_(); }

                friend std::ostream &operator<<(std::ostream &, const BT9Reader::EdgeTable &);

                friend class BT9Reader;

            private:
                void print_(std::ostream &os) const {
                    assert(bt9_reader_ != nullptr);

                    os << "BT9_EDGES\n";
                    os << "#EDGE  "
                       << std::setw(4) << "  id"
                       << std::setw(4) << "  src_id "
                       << std::setw(4) << "  dest_id"
                       << std::setw(8) << "taken "
                       << std::setw(20) << " br_virt_target "
                       << std::setw(20) << "  br_phy_target "
                       << std::setw(8) << "  inst_cnt "
                       << '\n';

                    for (const auto &edge : bt9_reader_->edge_table) {
                        os << "EDGE " << edge << '\n';
                    }
                }

                EdgeTable(const BT9Reader *rd) : bt9_reader_(rd) {};

                EdgeTable(const EdgeTable &) = delete;

                EdgeTable(EdgeTable &&) = delete;

                EdgeTable &operator=(const EdgeTable &) = delete;

                const BT9Reader *bt9_reader_ = nullptr;
        };

        class BranchInstanceIterator;

        friend class BranchInstanceIterator;

        /*!
         * \class BranchInstanceIterator
         * \brief This is the iterator for internal branch edge sequence list.
         * \note This is supposed to be a forward(input) iterator
         */
        class BranchInstanceIterator {
            public:
                /*!
                 * \brief Default constructor
                 *
                 * It's constructed to be an end iterator by default if no BT9Reader is provided
                 */
                BranchInstanceIterator() :
                        bt9_reader_(nullptr),
                        reach_end_(true) {}

                /*!
                 * \brief Constructor
                 * \param rd Pointer to its associated BT9 reader
                 * \param end Indicate if it points to the end of edge sequence list
                 */
                BranchInstanceIterator(BT9Reader *rd,
                                       bool end = false) :
                        bt9_reader_(rd),
                        reach_end_(end) {
                }

                /*!
                 * \brief Copy constructor
                 * \param rhs The other BranchInstanceIterator instance to copy from
                 */
                BranchInstanceIterator(const BranchInstanceIterator &rhs) :
                        bt9_reader_(rhs.bt9_reader_),
                        reach_end_(rhs.reach_end_),
                        index_(rhs.index_),
                        br_inst_(rhs.br_inst_) {
                }

                /*!
                 * \brief Move constructor
                 * \param rhs The other BranchInstanceIterator instance to move from
                 */
                BranchInstanceIterator(BranchInstanceIterator &&rhs) :
                        bt9_reader_(rhs.bt9_reader_),
                        reach_end_(rhs.reach_end_),
                        index_(rhs.index_),
                        br_inst_(rhs.br_inst_) {
                }

                /*!
                 * \brief Copy assignment operator
                 * \param rhs The other BranchInstanceIterator instance to copy from
                 */
                BranchInstanceIterator &operator=(const BranchInstanceIterator &rhs) {
                    bt9_reader_ = rhs.bt9_reader_;
                    reach_end_ = rhs.reach_end_;
                    index_ = rhs.index_;
                    br_inst_ = rhs.br_inst_;

                    return *this;
                }

                /// Destructor
                ~BranchInstanceIterator() {}

                /*!
                 * \brief Pre-increment operator
                 * \note If the iterator access index goes beyond its current access window,
                 *       the window will be automatically shifted forward (half-buffer size stride)
                 *       by the BT9Reader library helper functions
                 */
                BranchInstanceIterator &operator++() {
                    br_inst_.invalidate_();
                    if (!reach_end_) {
                        reach_end_ = !bt9_reader_->moveToNextEdgeSeqListEntry_(index_);
                    }

                    return *this;
                }

                /*!
                 * \brief Post-increment operator
                 * \note If the iterator access index goes beyond its current access window,
                 *       the window will be automatically shifted forward (half-buffer size stride)
                 *       by the BT9Reader library helper functions
                 */
                BranchInstanceIterator operator++(int) {
                    auto temp = *this;

                    br_inst_.invalidate_();
                    if (!reach_end_) {
                        reach_end_ = !bt9_reader_->moveToNextEdgeSeqListEntry_(index_);
                    }

                    return *this;
                }

                /*!
                 * \brief Equal operator
                 * \note The end iterator comparison is treated separately because the iterator
                 *       itself is not aware of its position in the file until the BT9Reader
                 *       informs it about this
                 */
                bool operator==(const BranchInstanceIterator &rhs) const {
                    if (bt9_reader_ != rhs.bt9_reader_) {
                        return false;
                    } else if (!reach_end_ && !rhs.reach_end_) {
                        return (index_ == rhs.index_);
                    } else {
                        return (reach_end_ && rhs.reach_end_);
                    }
                }

                /// Not-equal operator
                bool operator!=(const BranchInstanceIterator &rhs) const {
                    return !this->operator==(rhs);
                }

                /*!
                 * \brief Dereference operator
                 * \note Bound-checking is done under the scene by the BT9Reader helper function
                 */
                BT9BranchInstance &operator*() {
                    if (!br_inst_.isValid()) {
                        bt9_reader_->loadBT9BranchInstance_(index_, br_inst_);
                    }

                    return br_inst_;
                }

                /*!
                 * \brief Dereference operator
                 * \note Bound-checking is done under the scene by the BT9Reader helper function
                 */
                BT9BranchInstance *operator->() {
                    if (!br_inst_.isValid()) {
                        bt9_reader_->loadBT9BranchInstance_(index_, br_inst_);
                    }

                    return &br_inst_;
                }

            private:
                BT9Reader *bt9_reader_ = nullptr;
                bool reach_end_ = false;
                uint64_t index_ = 0;
                BT9BranchInstance br_inst_;
        };

        BranchInstanceIterator begin() { return BranchInstanceIterator(this); }

        BranchInstanceIterator end() { return BranchInstanceIterator(this, true); }


    public:
        /// BT9 header
        BT9ReaderHeader header;

        /// Node table (wrapper class for the internal node table) that is visible to the user
        NodeTable node_table;

        /// Edge table (wrapper class for the internal edge table) that is visible to the user
        EdgeTable edge_table;


    private:
        /*!
         * \brief Open and read BT9 trace file via Linux pipe
         * \param name BT9 trace file path
         * \Return It returns the integer file descriptor that is required by the 
         *         boost::iostreams::stream_buffer
         */
        int openBT9TraceFile_(const std::string &name) {
            std::string cmd = "/bin/cat " + name;

            auto gzip_suffix_pos = name.find(".gz");
            if (gzip_suffix_pos != std::string::npos) {
                cmd = "gunzip -dc " + name;
            }

            FILE *pipe = popen(cmd.c_str(), "r");

            if (!pipe) {
                std::cerr << "Failed to open trace file \'"
                          << name << "\' with pipe\n";
                exit(-1);
            }

            return fileno(pipe);
        }

        /// Read BT9 tracefile header with linux pipe
        void readBT9Header_() {
            std::string line;
            std::string token;

            // Check BT9 header title line
            std::getline(pinfile_, line, '\n');
            line_num_++;

            std::stringstream ss(line);
            ss >> token;
            if (token != "BT9_SPA_TRACE_FORMAT") {
                std::cerr << "line:" << line_num_ << " \'" << tracefile_name_ << "\' is not BT9 file\n";
                exit(-1);
            }

            // Read BT9 header fields (each is a key-value string pair)
            while (std::getline(pinfile_, line, '\n')) {
                line_num_++;

                // Skip any comments at the end of line
                auto pos = line.find_first_of('#');
                if (pos != std::string::npos) {
                    line.erase(pos, std::numeric_limits<std::string::size_type>::max());
                }

                std::stringstream ss(line);
                std::string token;
                ss >> token;

                // Skip if the whole line contains only comments
                if (token.empty()) {
                    continue;
                }

                if (token == "BT9_NODES") {
                    reach_node_table_ = true;
                    break;
                } else {
                    // Erase the key-string, get the following value-string
                    line.erase(0, token.size());

                    // Update header fields
                    parseHeader_(ss, token, line);
                }
            }
        }

        /// Parse BT9 tracefile header
        void parseHeader_(std::stringstream &ss,
                          std::string &token,
                          std::string &line) {
            if (token == "bt9_minor_version:") {
                ss >> token;
                try {
                    header.version_num_ = static_cast<BasicHeader::BT9MinorVersionNum>(std::stoul(token, nullptr, 0));
                }
                catch (const std::invalid_argument &ex) {
                    std::cerr << ex.what();
                    std::cerr << "line:" << line_num_ << " bt9_minor_version: " << token << " is invalid!\n";
                    exit(-1);
                }
            } else if (token == "has_physical_address:") {
                ss >> token;
                try {
                    header.has_phy_addr_ = std::stoul(token, nullptr, 0);
                }
                catch (const std::invalid_argument &ex) {
                    std::cerr << ex.what();
                    std::cerr << "line:" << line_num_ << " has_physical_address: " << token << " is invalid!\n";
                    exit(-1);
                }
            } else if (token == "md5_checksum:") {
                header.md5sum_ = line;
            } else if (token == "conversion_date:") {
                header.date_ = line;
            } else if (token == "original_stf_input_file:") {
                header.original_tracefile_path_ = line;
            } else {
                header.unclassified_fields_[token] = line;
            }
        }

        /// Read BT9 tracefile node table with linux pipe
        void readBT9NodeTable_() {
            std::string line;
            std::string token;

            if (!reach_node_table_) {
                std::cerr << "\'BT9_NODES\' is missing!\n";
                exit(-1);
            }

            uint32_t max_id = 0;
            while (std::getline(pinfile_, line, '\n')) {
                line_num_++;

                // Skip any comments at the end of line
                std::string comments;
                auto pos = line.find_first_of('#');
                if (pos != std::string::npos) {
                    comments = line.substr(pos + 1);
                    line.erase(pos, std::numeric_limits<std::string::size_type>::max());
                }

                std::stringstream ss(line);
                std::string token;
                ss >> token;

                // Skip if the whole line contains only comments
                if (token.empty()) {
                    continue;
                }

                if (token == "BT9_EDGES") {
                    reach_edge_table_ = true;
                    break;
                } else if (token == "NODE") {
                    BT9ReaderNodeRecord node_record;

                    parseNodeRecordFixedFields_(node_record, ss, token);
                    parseNodeRecordOptionalFields_(node_record, ss, token);
                    parseNodeMnemonicsFromComments_(node_record, comments);
                    updateNodeTable_(node_record);

                    // Keep track of the maximum node id
                    max_id = std::max(max_id, node_record.id_);
                } else {
                    std::cerr << "line:" << line_num_ << " \'NODE\' specifier is missing!\n";
                    exit(-1);
                }
            }

            // Update node order vector
            updateNodeOrderVector_(++max_id);
        }

        /*!
         * \brief Parse branch mnemonics from comments (start with # sign)
         * \note Current implementation assumes branch mnemonics as part of a comments, not as a regular record field.
         *       This function might be deprecated in the future when branch mnemonics no longer appear as part of comments
         */
        void parseNodeMnemonicsFromComments_(BT9ReaderNodeRecord &node_record,
                                             std::string &comments) {
            std::stringstream ss(comments);
            std::string token;

            while (ss >> token) {
                if (token == "mnemonic:") {
                    ss >> token;

                    if (token.at(0) != '\"') {
                        std::cerr << "line:" << line_num_ << " missing \" at the beginning of branch mnemonic!\n";
                        exit(-1);
                    }

                    std::string str = token.substr(1);
                    bool end = false;
                    while (ss >> token) {
                        str += " ";
                        if (token.back() == '\"') {
                            str += token.substr(0, token.size() - 1);
                            end = true;
                            break;
                        }

                        str += token;
                    }

                    // Corner case
                    if (str.back() == '\"') {
                        end = true;
                        str = str.substr(0, str.size() - 1);
                    }

                    if (!end) {
                        std::cerr << "line:" << line_num_ << " missing \" at the end of branch mnemonic!\n";
                        //exit(-1);
                    }

                    node_record.mnemonic_ = str;
                }
            }
        }

        /// Parse the fixed fields of BT9 node record
        void parseNodeRecordFixedFields_(BT9ReaderNodeRecord &node_record,
                                         std::stringstream &ss,
                                         std::string &token) {
            uint64_t index = 0;
            while ((index < BT9ReaderNodeRecord::NUM_VALUE_FIELD_) && (ss >> token)) {
                switch (index) {
                    case 0: // id
                        try {
                            node_record.id_ = std::stoi(token, nullptr, 0);
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " node id: " << token << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                    case 1: // virtual_address
                        try {
                            node_record.br_virtual_addr_ = std::stoull(token, nullptr, 0);
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " virtual address: " << token << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                    case 2: // physical_address
                        if (token == "-") {
                            node_record.br_phy_addr_ = std::numeric_limits<uint64_t>::max();
                            node_record.br_phy_addr_valid_ = false;
                            ++index;
                        } else {
                            try {
                                node_record.br_phy_addr_ = std::stoull(token, nullptr, 0);
                                node_record.br_phy_addr_valid_ = true;
                                ++index;
                            }
                            catch (const std::invalid_argument &ex) {
                                std::cerr << ex.what();
                                std::cerr << "line:" << line_num_ << " physical address: " << token << " is invalid!\n";
                                exit(-1);
                            }
                        }
                        break;
                    case 3: // opcode
                        try {
                            node_record.opcode_ = std::stoull(token, nullptr, 0);
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " opcode: " << token << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                    case 4: // size
                        try {
                            node_record.opcode_size_ = std::stoi(token, nullptr, 0);
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " opcode size: " << token << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                }
            }
        }

        /// Parse the optional fields of BT9 node record
        void parseNodeRecordOptionalFields_(BT9ReaderNodeRecord &node_record,
                                            std::stringstream &ss,
                                            std::string &token) {
            while (ss >> token) {
                if (token == "class:") {
                    ss >> token;
                    try {
                        node_record.br_class_.parseBrClass(token);
                    }
                    catch (const std::invalid_argument &ex) {
                        std::cerr << ex.what();
                        std::cerr << "line:" << line_num_ << " BrClass: " << token << " is invalid!\n";
                        exit(-1);
                    }
                } else if (token == "behavior:") {
                    ss >> token;
                    try {
                        node_record.br_behavior_.parseBrBehavior(token);
                    }
                    catch (const std::invalid_argument &ex) {
                        std::cerr << ex.what();
                        std::cerr << "line:" << line_num_ << " BrBehavior: " << token << " is invalid!\n";
                        exit(-1);
                    }
                } else if (token == "taken_cnt:") {
                    ss >> token;
                    try {
                        node_record.br_taken_cnt_ = std::stoul(token, nullptr, 0);
                    }
                    catch (const std::invalid_argument &ex) {
                        std::cerr << ex.what();
                        std::cerr << "line:" << line_num_ << " taken_cnt: " << token << " is invalid!\n";
                        exit(-1);
                    }
                } else if (token == "not_taken_cnt:") {
                    ss >> token;
                    try {
                        node_record.br_untaken_cnt_ = std::stoul(token, nullptr, 0);
                    }
                    catch (const std::invalid_argument &ex) {
                        std::cerr << ex.what();
                        std::cerr << "line:" << line_num_ << " not_taken_cnt: " << token << " is invalid!\n";
                        exit(-1);
                    }
                } else if (token == "tgt_cnt:") {
                    ss >> token;
                    try {
                        node_record.br_tgt_cnt_ = std::stoul(token, nullptr, 0);
                    }
                    catch (const std::invalid_argument &ex) {
                        std::cerr << ex.what();
                        std::cerr << "line:" << line_num_ << " tgt_cnt: " << token << " is invalid!\n";
                        exit(-1);
                    }
                } else {
                    std::string key = token;
                    ss >> token;
                    node_record.unclassified_fields_[key] = token;
                }
            }
        }

        /// Update the internal node table of BT9 reader
        void updateNodeTable_(BT9ReaderNodeRecord &node_record) {
            NodeTableHashKey node_hash_key = node_record.br_virtual_addr_;
            if (node_record.id_ == 0) {
                // dummy source node
                node_hash_key = std::numeric_limits<uint64_t>::max() - 1;
            }

            if (node_table_.find(node_hash_key) == node_table_.end()) {
                node_table_[node_hash_key] = node_record;
            } else {
                std::cerr << "line:" << line_num_ << " duplicated node: " << std::hex << std::showbase << node_hash_key
                          << std::dec << std::noshowbase << " is detected!\n";
                exit(-1);
            }
        }

        /// Update the internal node order vector of BT9 reader
        void updateNodeOrderVector_(uint32_t size) {
            node_order_vector_.resize(size);
            for (auto &node : node_table_) {
                node_order_vector_[node.second.id_] = &node.second;
            }
        }

        /// Read BT9 tracefile edge table with linux pipe
        void readBT9EdgeTable_() {
            std::string line;
            std::string token;

            if (!reach_edge_table_) {
                std::cerr << "\'BT9_EDGES\' is missing!\n";
                exit(-1);
            }

            uint32_t max_id = 0;
            while (std::getline(pinfile_, line, '\n')) {
                line_num_++;

                auto pos = line.find_first_of('#');
                if (pos != std::string::npos) {
                    line.erase(pos, std::numeric_limits<std::string::size_type>::max());
                }

                std::stringstream ss(line);
                std::string token;
                ss >> token;

                if (token.empty()) {
                    continue;
                }

                if (token == "BT9_EDGE_SEQUENCE") {
                    reach_edge_seq_list_ = true;
                    start_line_num_ = line_num_;
                    break;
                } else if (token == "EDGE") {
                    BT9ReaderEdgeRecord edge_record;

                    parseEdgeRecordFixedFields_(edge_record, ss, token);
                    parseEdgeRecordOptionalFields_(edge_record, ss, token);
                    updateEdgeTable_(edge_record);

                    max_id = std::max(max_id, edge_record.id_);
                } else {
                    std::cerr << "line:" << line_num_ << " \'EDGE\' specifier is missing!\n";
                    exit(-1);
                }
            }

            // Update edge order vector
            updateEdgeOrderVector_(++max_id);
        }

        /// Parse the fixed fields of BT9 edge record
        void parseEdgeRecordFixedFields_(BT9ReaderEdgeRecord &edge_record,
                                         std::stringstream &ss,
                                         std::string &token) {
            uint64_t index = 0;
            while ((index < BT9ReaderEdgeRecord::NUM_VALUE_FIELD_) && (ss >> token)) {
                switch (index) {
                    case 0: // edge_id
                        try {
                            edge_record.id_ = std::stoi(token, nullptr, 0);
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " edge id: " << token << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                    case 1: // src_node_id
                        try {
                            edge_record.src_node_id_ = std::stoi(token, nullptr, 0);

                            if (!isValidNodeIndex_(edge_record.src_node_id_)) {
                                throw std::invalid_argument("Invalid token detected!\n");
                            }
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " source node id: " << token << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                    case 2: // dest_node_id
                        try {
                            edge_record.dest_node_id_ = std::stoi(token, nullptr, 0);
                            if (!isValidNodeIndex_(edge_record.dest_node_id_)) {
                                throw std::invalid_argument("Invalid token detected!\n");
                            }
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " destination node id: " << token << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                    case 3: // br_is_taken?
                        if (token == "T") {
                            edge_record.is_taken_path_ = true;
                            ++index;
                        } else if (token == "N") {
                            edge_record.is_taken_path_ = false;
                            ++index;
                        } else {
                            std::cerr << "line:" << line_num_ << " branch taken indicator: " << token
                                      << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                    case 4: // br_virtual_target
                        try {
                            edge_record.br_virtual_tgt_ = std::stoull(token, nullptr, 0);
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " branch virtual target: " << token
                                      << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                    case 5: // br_physical_target
                        if (token == "-") {
                            edge_record.br_phy_tgt_ = std::numeric_limits<uint64_t>::max();
                            edge_record.br_phy_tgt_valid_ = false;
                            ++index;
                        } else {
                            try {
                                edge_record.br_phy_tgt_ = std::stoull(token, nullptr, 0);
                                edge_record.br_phy_tgt_valid_ = true;
                                ++index;
                            }
                            catch (const std::invalid_argument &ex) {
                                std::cerr << ex.what();
                                std::cerr << "line:" << line_num_ << " branch physical target: " << token
                                          << " is invalid!\n";
                                exit(-1);
                            }
                        }
                        break;
                    case 6: // non-branch instruction count
                        try {
                            edge_record.inst_cnt_ = std::stoull(token, nullptr, 0);
                            ++index;
                        }
                        catch (const std::invalid_argument &ex) {
                            std::cerr << ex.what();
                            std::cerr << "line:" << line_num_ << " non-branch instruction count: " << token
                                      << " is invalid!\n";
                            exit(-1);
                        }
                        break;
                }
            }
        }

        /// Parse the optional fields of BT9 edge record
        void parseEdgeRecordOptionalFields_(BT9ReaderEdgeRecord &edge_record,
                                            std::stringstream &ss,
                                            std::string &token) {
            while (ss >> token) {
                if (token == "traverse_cnt:") {
                    ss >> token;
                    try {
                        edge_record.observed_traverse_cnt_ = std::stoul(token, nullptr, 0);
                    }
                    catch (const std::invalid_argument &ex) {
                        std::cerr << ex.what();
                        std::cerr << "line:" << line_num_ << " traverse_cnt: " << token << " is invalid!\n";
                        exit(-1);
                    }
                } else {
                    std::string key = token;
                    ss >> token;
                    edge_record.unclassified_fields_[key] = token;
                }
            }
        }

        /// Update the internal edge table of BT9 reader
        void updateEdgeTable_(BT9ReaderEdgeRecord &edge_record) {
            const bool is_taken = edge_record.is_taken_path_;
            const uint64_t src_br_virtual_pc = node_order_vector_[edge_record.src_node_id_]->br_virtual_addr_;
            const uint64_t dest_br_virtual_pc = is_taken ? edge_record.br_virtual_tgt_ : 0;
            EdgeTableHashKey edge_hash_key = {src_br_virtual_pc, dest_br_virtual_pc};

            const bool is_last_dummy_edge = (node_order_vector_[edge_record.dest_node_id_]->opcode_size_ == 0);
            if (is_last_dummy_edge) {
                edge_hash_key = {src_br_virtual_pc, std::numeric_limits<uint64_t>::max()};
            }

            if (edge_table_.find(edge_hash_key) == edge_table_.end()) {
                edge_table_[edge_hash_key] = edge_record;
            } else {
                std::cerr << "line:" << line_num_ << " duplicated edge: (" << std::hex << std::showbase
                          << edge_hash_key.first << ", " << edge_hash_key.second << std::dec << std::noshowbase
                          << ") detected!\n";
                exit(-1);
            }
        }

        /// Update the internal edge order vector of BT9 reader
        void updateEdgeOrderVector_(uint32_t size) {
            edge_order_vector_.resize(size);
            for (auto &edge : edge_table_) {
                edge_order_vector_[edge.second.id_] = &edge.second;
            }
        }

        /*! 
         * \brief Check the validity of supplied node index
         * \param idx Node record index
         * \return Returns false if the node referred to by idx doesn't exist in node table
         */
        bool isValidNodeIndex_(uint32_t idx) const {
            return ((idx < node_order_vector_.size()) && (node_order_vector_[idx] != nullptr));
        }

        /*!
         * \brief Check the validity of supplied edge index
         * \param idx Edge record index
         * \return Returns false if the edge referred to by idx doesn't exist in edge table
         */
        bool isValidEdgeIndex_(uint32_t idx) const {
            return ((idx < edge_order_vector_.size()) && (edge_order_vector_[idx] != nullptr));
        }

        /*!
         * \brief Read the next non-comment line inside the edge sequence list
         * \param token The next line inside edge sequence list
         * \note It automatically filters out those lines that may contain comments only
         */
        void readNextNonCommentLine_(std::string &token) {
            std::string line;
            do {
                if (!std::getline(pinfile_, line, '\n')) {
                    break;
                }
                line_num_++;

                // Skip any comments on the same line
                auto pos = line.find_first_of('#');
                if (pos != std::string::npos) {
                    line.erase(pos, std::numeric_limits<std::string::size_type>::max());
                }

                std::stringstream ss(line);
                ss >> token;

                // Skip any comments occupying the whole line
            } while (token.empty());
        }

        /*! 
         * \brief Read the next valid entry in the edge sequence list
         * \param edge_id Next edge sequence list entry (valid only when return value is true)
         * \return Returns false if it already reaches the end of file.
         * \note It throws std::invalid_argument exception if the readout edge sequence list 
         *       entry is invalid.
         */
        bool readNextEdgeSequenceListEntry_(uint32_t &edge_id) {
            std::string token;

            readNextNonCommentLine_(token);

            if (!token.empty() && token != "EOF") {
                try {
                    // Check if the token is a valid number
                    edge_id = std::stoi(token, nullptr, 0);

                    // Check if the number is a valid edge index
                    if (!isValidEdgeIndex_(edge_id)) {
                        throw std::invalid_argument("Invalid Edge Index!\n");
                    }
                }
                catch (const std::invalid_argument &ex) {
                    std::cerr << ex.what();
                    std::cerr << "line:" << line_num_
                              << " edge id: " << token << " in edge sequence list is invalid!\n";
                    exit(-1);
                }

                return true;
            }

            return false;
        }

        /*!
         * \brief Initialize BT9 edge sequence list access window
         * \note The window size can be configured when BT9Reader instance is constructed.
         */
        void initBT9EdgeSeqListAccessWindow_() {
            if (!reach_edge_seq_list_) {
                std::cerr << "\'BT9_EDGE_SEQUENCE\' is missing!\n";
                exit(-1);
            }

            uint64_t buffer_size = buffer_.size();
            while (buffer_end_ < buffer_begin_ + buffer_size) {
                uint32_t edge_id = 0;
                if (!readNextEdgeSequenceListEntry_(edge_id)) {
                    reach_eof_ = true;
                    break;
                }

                buffer_[buffer_end_ % buffer_size] = edge_id;
                buffer_end_++;
            }
        }

        /*!
         * \brief Shift BT9 edge sequence list access window foward
         * \note Forward shifting stride is half buffer size
         */
        void shiftBT9EdgeSeqListAccessWindow_() {
            assert(!reach_eof_);

            uint64_t buffer_size = buffer_.size();
            buffer_begin_ += (buffer_size >> 1);
            while (buffer_end_ < buffer_begin_ + buffer_size) {
                uint32_t edge_id = 0;
                if (!readNextEdgeSequenceListEntry_(edge_id)) {
                    reach_eof_ = true;
                    break;
                }

                buffer_[buffer_end_ % buffer_size] = edge_id;
                buffer_end_++;
            }
        }

        /*!
         * \brief Helper function provided by BT9Reader to increment BranchInstanceIterator
         * \param idx The iterator access index
         * \return Returns false if the iterator reaches the end of file
         */
        bool moveToNextEdgeSeqListEntry_(uint64_t &idx) {
            idx++;

            if (idx >= buffer_end_) {
                if (reach_eof_) {
                    return false;
                } else {
                    shiftBT9EdgeSeqListAccessWindow_();
                }
            }

            return true;
        }

        /*!
         * \brief Helper function provided by BT9Reader to do iterator access bound-checking
         * \param idx The iterator access index
         * \note It throws std::out_of_range exception if either overflow or underflow occurs
         */
        void edgeSeqListAccessIndexBoundChecking_(const uint64_t &idx) {
            if (idx < buffer_begin_) {
                throw std::out_of_range("Edge sequence list access window underflow!\n");
            }

            if (reach_eof_ && (idx >= buffer_end_)) {
                throw std::out_of_range("Edge sequence list access window overflow!\n");
            }
        }

        /*!
         * \brief Helper function provided by BT9Reader to get the edge sequence list entry
         * \param idx The iterator access index
         */
        const uint32_t &getEdgeSeqListEntry_(const uint64_t &idx) {
            return buffer_[idx % buffer_.size()];
        }

        /*!
         * \brief Helper function provided by BT9Reader to load branch instance if it's the
         *        first access by the iterator.
         * \param idx The iterator access index
         * \param br_inst The branch instance bufferred inside the iterator
         */
        void loadBT9BranchInstance_(const uint64_t &idx, BT9BranchInstance &br_inst) {
            edgeSeqListAccessIndexBoundChecking_(idx);

            const auto &edge_id = getEdgeSeqListEntry_(idx);
            const auto &edge_rec_ptr = edge_order_vector_[edge_id];
            const auto &src_node_rec_ptr = node_order_vector_[edge_rec_ptr->src_node_id_];
            const auto &dest_node_rec_ptr = node_order_vector_[edge_rec_ptr->dest_node_id_];

            br_inst.update_(src_node_rec_ptr, dest_node_rec_ptr, edge_rec_ptr);
        }

        /*!
         * \brief Return iterator to the beginning of internal node table
         * \note This is for internal use by the BT9Reader only
         */
        NodeTableIterator nodeTableBegin_() const {
            return NodeTableIterator(this, 0);
        }

        /*!
         * \brief Return iterator to the end of internal node table
         * \note This is for internal use by the BT9Reader only
         */
        NodeTableIterator nodeTableEnd_() const {
            return NodeTableIterator(this, node_order_vector_.size());
        }

        /*!
         * \brief Return iterator to the beginning of internal edge table
         * \note This is for internal use by the BT9Reader only
         */
        EdgeTableIterator edgeTableBegin_() const {
            return EdgeTableIterator(this, 0);
        }

        /*!
         * \brief Return iterator to the end of internal edge table
         * \note This is for internal use by the BT9Reader only
         */
        EdgeTableIterator edgeTableEnd_() const {
            return EdgeTableIterator(this, edge_order_vector_.size());
        }


        /// BT9 trace file name
        std::string tracefile_name_;

        /// BT9 edge sequence list starting position line number
        uint64_t start_line_num_ = 0;

        /// BT9 trace file line number
        uint64_t line_num_ = 0;

        /// BT9 internal node look-up table
        std::unordered_map<NodeTableHashKey, BT9ReaderNodeRecord> node_table_;

        /// BT9 internal node id tracking table
        std::vector<BT9ReaderNodeRecord *> node_order_vector_;

        /// Indicate if reading stream reaches node table
        bool reach_node_table_ = false;

        /// BT9 internal edge look-up table
        std::unordered_map<EdgeTableHashKey, BT9ReaderEdgeRecord> edge_table_;

        /// BT9 internal edge id tracking table
        std::vector<BT9ReaderEdgeRecord *> edge_order_vector_;

        /// Indicate if reading stream reaches edge table
        bool reach_edge_table_ = false;

        /// Indicate if reading stream reaches edge sequence list
        bool reach_edge_seq_list_ = false;

        /// Indicate if reading stream reaches end of file
        bool reach_eof_ = false;

        /// Boost iostreams stream buffer that could be constructed from a file descriptor
        boost::iostreams::stream_buffer <boost::iostreams::file_descriptor_source> fpstream_;

        /// BT9 reader istream handle
        std::istream pinfile_;

        /// BT9 reader edge sequence list access window
        std::vector<uint32_t> buffer_;

        /// Index of edge sequence entry that is currently the first entry of access window
        uint64_t buffer_begin_ = 0;

        /// Index of edge sequence entry that is currently the last entry of access window
        uint64_t buffer_end_ = 0;

};

/*!
 * \brief Overloaded output operator for user-visible BT9Reader::NodeTable
 * \note BT9Reader::NodeTable is a wrapper class of node table internal to BT9Reader
 */
inline std::ostream &operator<<(std::ostream &os, const BT9Reader::NodeTable &table) {
    table.print_(os);
    return os;
}

/*!
 * \brief Overloaded output operator for user-visible BT9Reader::NodeTable
 * \note BT9Reader::NodeTable is a wrapper class of node table internal to BT9Reader
 */
inline std::ostream &operator<<(std::ostream &os, const BT9Reader::EdgeTable &table) {
    table.print_(os);
    return os;
}
}

namespace std {
template<>
struct iterator_traits<bt9::BT9Reader::NodeTableIterator> {
    using value_type = bt9::BT9ReaderNodeRecord;
    using difference_type = int64_t;
    using iterator_category = random_access_iterator_tag;
    using pointer = bt9::BT9ReaderNodeRecord *;
    using reference = bt9::BT9ReaderNodeRecord &;
};

template<>
struct iterator_traits<bt9::BT9Reader::EdgeTableIterator> {
    using value_type = bt9::BT9ReaderEdgeRecord;
    using difference_type = int64_t;
    using iterator_category = random_access_iterator_tag;
    using pointer = bt9::BT9ReaderEdgeRecord *;
    using reference = bt9::BT9ReaderEdgeRecord &;
};

template<>
struct iterator_traits<bt9::BT9Reader::BranchInstanceIterator> {
    using value_type = bt9::BT9BranchInstance;
    using difference_type = ptrdiff_t;
    using iterator_category = input_iterator_tag;
    using pointer = bt9::BT9BranchInstance *;
    using reference = bt9::BT9BranchInstance &;
};
}



// __BT9_READER_H__
#endif
