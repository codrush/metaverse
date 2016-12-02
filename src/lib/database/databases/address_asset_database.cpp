/**
 * Copyright (c) 2011-2015 mvs developers (see AUTHORS)
 *
 * This file is part of mvsd.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/databases/address_asset_database.hpp>
//#include <bitcoin/bitcoin/chain/attachment/account/address_asset.hpp>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_multimap_iterable.hpp>
#include <bitcoin/database/primitives/record_multimap_iterator.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;

BC_CONSTEXPR size_t number_buckets = 1000;
BC_CONSTEXPR size_t header_size = record_hash_table_header_size(number_buckets);
BC_CONSTEXPR size_t initial_lookup_file_size = header_size + minimum_records_size;

BC_CONSTEXPR size_t record_size = hash_table_multimap_record_size<short_hash>();

BC_CONSTEXPR size_t asset_transfer_record_size = 1 + 36 + 4 + 8 + 2 + ASSET_DETAIL_FIX_SIZE; // ASSET_DETAIL_FIX_SIZE is the biggest one
//		+ std::max({ETP_FIX_SIZE, ASSET_DETAIL_FIX_SIZE, ASSET_TRANSFER_FIX_SIZE});
BC_CONSTEXPR size_t row_record_size = hash_table_record_size<hash_digest>(asset_transfer_record_size);

address_asset_database::address_asset_database(const path& lookup_filename,
    const path& rows_filename, std::shared_ptr<shared_mutex> mutex)
  : lookup_file_(lookup_filename, mutex), 
    lookup_header_(lookup_file_, number_buckets),
    lookup_manager_(lookup_file_, header_size, record_size),
    lookup_map_(lookup_header_, lookup_manager_),
    rows_file_(rows_filename, mutex),
    rows_manager_(rows_file_, 0, row_record_size),
    rows_list_(rows_manager_),
    rows_multimap_(lookup_map_, rows_list_)
{
}

// Close does not call stop because there is no way to detect thread join.
address_asset_database::~address_asset_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool address_asset_database::create()
{
    // Resize and create require a started file.
    if (!lookup_file_.start() ||
        !rows_file_.start())
        return false;

    // These will throw if insufficient disk space.
    lookup_file_.resize(initial_lookup_file_size);
    rows_file_.resize(minimum_records_size);

    if (!lookup_header_.create() ||
        !lookup_manager_.create() ||
        !rows_manager_.create())
        return false;

    // Should not call start after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start() &&
        rows_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool address_asset_database::start()
{
    return
        lookup_file_.start() &&
        rows_file_.start() &&
        lookup_header_.start() &&
        lookup_manager_.start() &&
        rows_manager_.start();
}

bool address_asset_database::stop()
{
    return
        lookup_file_.stop() &&
        rows_file_.stop();
}

bool address_asset_database::close()
{
    return
        lookup_file_.close() &&
        rows_file_.close();
}

// ----------------------------------------------------------------------------

void address_asset_database::store_output(const short_hash& key, const output_point& outpoint, 
	uint32_t output_height, uint64_t value, uint16_t business_kd, etp& business_data)
{
	auto write = [&](memory_ptr data)
	{
		auto serial = make_serializer(REMAP_ADDRESS(data));
		serial.write_byte(static_cast<uint8_t>(point_kind::output));
		serial.write_data(outpoint.to_data());
		serial.write_4_bytes_little_endian(output_height);
		serial.write_8_bytes_little_endian(value);
		serial.write_2_bytes_little_endian(business_kd);
		serial.write_data(business_data.to_data());
	};
	rows_multimap_.add_row(key, write);
}

void address_asset_database::store_output(const short_hash& key, const output_point& outpoint, 
	uint32_t output_height, uint64_t value, uint16_t business_kd, asset_detail& business_data)
{
	auto write = [&](memory_ptr data)
	{
		auto serial = make_serializer(REMAP_ADDRESS(data));
		serial.write_byte(static_cast<uint8_t>(point_kind::output));
		serial.write_data(outpoint.to_data());
		serial.write_4_bytes_little_endian(output_height);
		serial.write_8_bytes_little_endian(value);
		serial.write_2_bytes_little_endian(business_kd);
		serial.write_data(business_data.to_data());
	};
	rows_multimap_.add_row(key, write);
}

void address_asset_database::store_output(const short_hash& key, const output_point& outpoint, 
	uint32_t output_height, uint64_t value, uint16_t business_kd, asset_transfer& business_data)
{
	auto write = [&](memory_ptr data)
	{
		auto serial = make_serializer(REMAP_ADDRESS(data));
		serial.write_byte(static_cast<uint8_t>(point_kind::output));
		serial.write_data(outpoint.to_data());
		serial.write_4_bytes_little_endian(output_height);
		serial.write_8_bytes_little_endian(value);
		serial.write_2_bytes_little_endian(business_kd);
		serial.write_data(business_data.to_data());
	};
	rows_multimap_.add_row(key, write);
}



void address_asset_database::store_input(const short_hash& key,
    const output_point& inpoint, uint32_t input_height,
    const input_point& previous)
{
    auto write = [&](memory_ptr data)
    {
        auto serial = make_serializer(REMAP_ADDRESS(data));
        serial.write_byte(static_cast<uint8_t>(point_kind::spend));
        serial.write_data(inpoint.to_data());
        serial.write_4_bytes_little_endian(input_height);
        serial.write_8_bytes_little_endian(previous.checksum());
    };
    rows_multimap_.add_row(key, write);
}


void address_asset_database::delete_last_row(const short_hash& key)
{
    rows_multimap_.delete_last_row(key);
}
/// get all record of key from database
business_record::list address_asset_database::get(const short_hash& key,
    size_t limit, size_t from_height) const
{
    // Read the height value from the row.
    const auto read_height = [](uint8_t* data)
    {
        static constexpr file_offset height_position = 1 + 36;
        const auto height_address = data + height_position;
        return from_little_endian_unsafe<uint32_t>(height_address);
    };

    // Read a row from the data for the history list.
    const auto read_row = [](uint8_t* data)
    {
        auto deserial = make_deserializer_unsafe(data);
        return business_record
        {
            // output or spend?
            static_cast<point_kind>(deserial.read_byte()),

            // point
            point::factory_from_data(deserial),

            // height
            deserial.read_4_bytes_little_endian(),

            // value or checksum
            { deserial.read_8_bytes_little_endian() },
            
            business_data::factory_from_data(deserial)
        };
    };

    business_record::list result;
    const auto start = rows_multimap_.lookup(key);
    const auto records = record_multimap_iterable(rows_list_, start);

    for (const auto index: records)
    {
        // Stop once we reach the limit (if specified).
        if (limit > 0 && result.size() >= limit)
            break;

        // This obtains a remap safe address pointer against the rows file.
        const auto record = rows_list_.get(index);
        const auto address = REMAP_ADDRESS(record);

        // Skip rows below from_height.
        if (from_height == 0 || read_height(address) >= from_height)
            result.emplace_back(read_row(address));
    }

    // TODO: we could sort result here.
    return result;
}

business_history::list address_asset_database::get_business_history(const short_hash& key,
		size_t limit, size_t from_height) const
{
	business_record::list compact = get(key, limit, from_height);
	
    business_history::list result;

    // Process and remove all outputs.
    for (auto output = compact.begin(); output != compact.end();)
    {
        if (output->kind == point_kind::output)
        {
            business_history row;
            row.output = output->point;
            row.output_height = output->height;
            row.value = output->value;
            row.spend = { null_hash, max_uint32 };
            row.temporary_checksum = output->point.checksum();
			row.data = output->data;
            result.emplace_back(row);
            output = compact.erase(output);
            continue;
        }

        ++output;
    }

    // All outputs have been removed, process the spends.
    for (const auto& spend: compact)
    {
        auto found = false;

        // Update outputs with the corresponding spends.
        for (auto& row: result)
        {
            if (row.temporary_checksum == spend.previous_checksum &&
                row.spend.hash == null_hash)
            {
                row.spend = spend.point;
                row.spend_height = spend.height;
                found = true;
                break;
            }
        }

        // This will only happen if the history height cutoff comes between
        // an output and its spend. In this case we return just the spend.
        if (!found)
        {
            business_history row;
            row.output = { null_hash, max_uint32 };
            row.output_height = max_uint64;
            row.value = max_uint64;
            row.spend = spend.point;
            row.spend_height = spend.height;
            result.emplace_back(row);
        }
    }

    compact.clear();

    // Clear all remaining checksums from unspent rows.
    for (auto& row: result)
        if (row.spend.hash == null_hash)
            row.spend_height = max_uint64;

    // TODO: sort by height and index of output, spend or both in order.
    return result;
}

void address_asset_database::sync()
{
    lookup_manager_.sync();
    rows_manager_.sync();
}

address_asset_statinfo address_asset_database::statinfo() const
{
    return
    {
        lookup_header_.size(),
        lookup_manager_.count(),
        rows_manager_.count()
    };
}

} // namespace database
} // namespace libbitcoin

