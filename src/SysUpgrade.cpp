/*
 * Copyright (c) 2023 Manjeet Singh <itsmanjeet1998@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "SysUpgrade.h"

#include "Error.h"
#include "Executor.h"
#include "Http.h"
#include "picosha2.h"
#include <format>
#include <fstream>
#include <random>

#define BLOCK_SIZE 4096

std::string random_string(int length) {
    const std::string VALID_CHARS =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, VALID_CHARS.size() - 1);

    std::string buffer;
    buffer.reserve(length);

    for (int i = 0; i < length; ++i) { buffer += VALID_CHARS[dis(gen)]; }

    return buffer;
}

static char* _formatted_time_remaining_from_seconds(guint64 seconds_remaining) {
    guint64 minutes_remaining = seconds_remaining / 60;
    guint64 hours_remaining = minutes_remaining / 60;
    guint64 days_remaining = hours_remaining / 24;

    GString* description = g_string_new(NULL);

    if (days_remaining)
        g_string_append_printf(
                description, "%" G_GUINT64_FORMAT " days ", days_remaining);

    if (hours_remaining)
        g_string_append_printf(description, "%" G_GUINT64_FORMAT " hours ",
                hours_remaining % 24);

    if (minutes_remaining)
        g_string_append_printf(description, "%" G_GUINT64_FORMAT " minutes ",
                minutes_remaining % 60);

    g_string_append_printf(description, "%" G_GUINT64_FORMAT " seconds ",
            seconds_remaining % 60);

    return g_string_free(description, FALSE);
}

void progress_callback(OstreeAsyncProgress* progress, gpointer user_data) {
    g_autofree char* status = NULL;
    gboolean caught_error, scanning;
    guint outstanding_fetches;
    guint outstanding_metadata_fetches;
    guint outstanding_writes;
    guint n_scanned_metadata;
    guint fetched_delta_parts;
    guint total_delta_parts;
    guint fetched_delta_part_fallbacks;
    guint total_delta_part_fallbacks;

    g_autoptr(GString) buf = g_string_new("");

    ostree_async_progress_get(progress, "outstanding-fetches", "u",
            &outstanding_fetches, "outstanding-metadata-fetches", "u",
            &outstanding_metadata_fetches, "outstanding-writes", "u",
            &outstanding_writes, "caught-error", "b", &caught_error, "scanning",
            "u", &scanning, "scanned-metadata", "u", &n_scanned_metadata,
            "fetched-delta-parts", "u", &fetched_delta_parts,
            "total-delta-parts", "u", &total_delta_parts,
            "fetched-delta-fallbacks", "u", &fetched_delta_part_fallbacks,
            "total-delta-fallbacks", "u", &total_delta_part_fallbacks, "status",
            "s", &status, NULL);

    if (*status != '\0') {
        g_string_append(buf, status);
    } else if (caught_error) {
        g_string_append_printf(
                buf, "Caught error, waiting for outstanding tasks");
    } else if (outstanding_fetches) {
        guint64 bytes_transferred, start_time, total_delta_part_size;
        guint fetched, metadata_fetched, requested;
        guint64 current_time = g_get_monotonic_time();
        g_autofree char* formatted_bytes_transferred = NULL;
        g_autofree char* formatted_bytes_sec = NULL;
        guint64 bytes_sec;

        /* Note: This is not atomic wrt the above getter call. */
        ostree_async_progress_get(progress, "bytes-transferred", "t",
                &bytes_transferred, "fetched", "u", &fetched,
                "metadata-fetched", "u", &metadata_fetched, "requested", "u",
                &requested, "start-time", "t", &start_time,
                "total-delta-part-size", "t", &total_delta_part_size, NULL);

        formatted_bytes_transferred =
                g_format_size_full(bytes_transferred, G_FORMAT_SIZE_DEFAULT);

        /* Ignore the first second, or when we haven't transferred any
         * data, since those could cause divide by zero below.
         */
        if ((current_time - start_time) < G_USEC_PER_SEC ||
                bytes_transferred == 0) {
            bytes_sec = 0;
            formatted_bytes_sec = g_strdup("-");
        } else {
            bytes_sec = bytes_transferred /
                        ((current_time - start_time) / G_USEC_PER_SEC);
            formatted_bytes_sec = g_format_size(bytes_sec);
        }

        /* Are we doing deltas?  If so, we can be more accurate */
        if (total_delta_parts > 0) {
            guint64 fetched_delta_part_size = ostree_async_progress_get_uint64(
                    progress, "fetched-delta-part-size");
            g_autofree char* formatted_fetched = NULL;
            g_autofree char* formatted_total = NULL;

            /* Here we merge together deltaparts + fallbacks to avoid bloating
             * the text UI */
            fetched_delta_parts += fetched_delta_part_fallbacks;
            total_delta_parts += total_delta_part_fallbacks;

            formatted_fetched = g_format_size(fetched_delta_part_size);
            formatted_total = g_format_size(total_delta_part_size);

            if (bytes_sec > 0) {
                guint64 est_time_remaining = 0;
                if (total_delta_part_size > fetched_delta_part_size)
                    est_time_remaining =
                            (total_delta_part_size - fetched_delta_part_size) /
                            bytes_sec;
                g_autofree char* formatted_est_time_remaining =
                        _formatted_time_remaining_from_seconds(
                                est_time_remaining);
                /* No space between %s and remaining, since
                 * formatted_est_time_remaining has a trailing space */
                g_string_append_printf(buf,
                        "Receiving delta parts: %u/%u %s/%s %s/s %sremaining",
                        fetched_delta_parts, total_delta_parts,
                        formatted_fetched, formatted_total, formatted_bytes_sec,
                        formatted_est_time_remaining);
            } else {
                g_string_append_printf(buf,
                        "Receiving delta parts: %u/%u %s/%s",
                        fetched_delta_parts, total_delta_parts,
                        formatted_fetched, formatted_total);
            }
        } else if (scanning || outstanding_metadata_fetches) {
            g_string_append_printf(buf,
                    "Receiving metadata objects: %u/(estimating) %s/s %s",
                    metadata_fetched, formatted_bytes_sec,
                    formatted_bytes_transferred);
        } else {
            g_string_append_printf(buf,
                    "Receiving objects: %u%% (%u/%u) %s/s %s",
                    (guint)((((double)fetched) / requested) * 100), fetched,
                    requested, formatted_bytes_sec,
                    formatted_bytes_transferred);
        }
    } else if (outstanding_writes) {
        g_string_append_printf(buf, "Writing objects: %u", outstanding_writes);
    } else {
        g_string_append_printf(
                buf, "Scanning metadata: %u", n_scanned_metadata);
    }

    std::cout << "\r" << buf->str << std::flush;
}

std::optional<SysUpgrade::UpdateInfo> SysUpgrade::update(
        const Deployment& deployment, const std::string& url, bool dry_run) {
    GError* error = nullptr;
    std::unique_ptr<OstreeAsyncProgress, decltype(&g_object_unref)> progress(
            ostree_async_progress_new_and_connect(progress_callback, nullptr),
            g_object_unref);

    auto merged_deployment = ostree_sysroot_get_merge_deployment(
            sysroot->sysroot, sysroot->osname.c_str());

    std::vector<std::string> revisions;

    std::vector<std::string> refs;
    if (deployment.refspec.ends_with("/local")) {
        refs.emplace_back(
                std::format("x86_64/os/{}", deployment.channel).c_str());
        revisions.emplace_back(deployment.base_revision);
    } else {
        refs.emplace_back(deployment.refspec.c_str());
        revisions.emplace_back(deployment.revision);
    }
    for (auto const& [id, revision] : deployment.extensions) {
        refs.emplace_back(
                std::format("x86_64/extension/{}/{}", id, deployment.channel)
                        .c_str());
        revisions.emplace_back(revision);
    }

    std::vector<const char*> crefs;
    for (auto const& i : refs) crefs.emplace_back(i.data());
    crefs.push_back(nullptr);

    if (!ostree_repo_pull(sysroot->repo, sysroot->osname.c_str(),
                (char**)(crefs.data()),
                (dry_run ? OSTREE_REPO_PULL_FLAGS_COMMIT_ONLY
                         : OSTREE_REPO_PULL_FLAGS_NONE),
                progress.get(), nullptr, &error)) {
        throw Error(error);
    }
    if (progress) {
        ostree_async_progress_finish(progress.get());
        std::cout << std::endl;
    }

    auto updateInfo = UpdateInfo{};
    bool changed = false;
    int i = 0;
    std::vector<std::string> updated_revision;
    for (auto refspec : refs) {
        auto [ref_changed, ref_revision, ref_changelog] =
                get_changelog(sysroot->repo, refspec, revisions[i++]);
        updated_revision.emplace_back(ref_revision);
        if (ref_changed) {
            updateInfo.changelog += ref_changelog + "\n";
            changed = true;
        }
    }

    if (changed) {
        std::string revision;
        std::unique_ptr<GKeyFile, decltype(&g_key_file_unref)> origin(
                nullptr, g_key_file_unref);

        if (deployment.refspec.ends_with("/local")) {
            gboolean resume = false;
            if (!ostree_repo_prepare_transaction(
                        sysroot->repo, &resume, nullptr, &error))
                throw Error(error);

            std::unique_ptr<OstreeMutableTree, decltype(&g_object_unref)>
                    mutableTree(
                            ostree_mutable_tree_new_from_commit(sysroot->repo,
                                    deployment.base_revision.c_str(), &error),
                            g_object_unref);
            if (mutableTree == nullptr) throw Error(error);

            for (int i = 1; i < refs.size(); i++) {
                std::unique_ptr<GFile, decltype(&g_object_unref)> commit(
                        nullptr, g_object_unref);
                GFile* res;
                gchar* file;
                if (!ostree_repo_read_commit(sysroot->repo, crefs[i], &res,
                            &file, nullptr, &error)) {
                    throw Error(error);
                }
                commit.reset(res);
                g_free(file);

                if (!ostree_repo_write_directory_to_mtree(sysroot->repo,
                            commit.get(), mutableTree.get(), nullptr, nullptr,
                            &error))
                    throw Error(error);
            }
            GFile* res;
            if (!ostree_repo_write_mtree(sysroot->repo, mutableTree.get(), &res,
                        nullptr, &error))
                throw Error(error);
            std::unique_ptr<GFile, decltype(&g_object_unref)> root(
                    res, g_object_unref);
            gchar* commit_checksum;
            std::unique_ptr<GVariantDict, decltype(&g_variant_dict_unref)>
                    options(g_variant_dict_new(nullptr), g_variant_dict_unref);
            g_variant_dict_insert_value(options.get(), "rlxos.revision.core",
                    g_variant_new_string(updated_revision[0].c_str()));
            for (int i = 0; i < deployment.extensions.size(); i++) {
                g_variant_dict_insert_value(options.get(),
                        std::format("rlxos.revision.{}",
                                deployment.extensions[i].first)
                                .c_str(),
                        g_variant_new_string(updated_revision[i + 1].c_str()));
            }

            if (!ostree_repo_write_commit(sysroot->repo, nullptr, nullptr,
                        nullptr, g_variant_dict_end(options.get()),
                        OSTREE_REPO_FILE(res), &commit_checksum, nullptr,
                        &error)) {
                throw Error(error);
            }
            ostree_repo_transaction_set_ref(sysroot->repo, nullptr,
                    std::format("x86_64/os/local").c_str(), commit_checksum);
            OstreeRepoTransactionStats stats;
            if (!ostree_repo_commit_transaction(
                        sysroot->repo, &stats, nullptr, &error)) {
                throw Error(error);
            }

            char* out_rev;
            if (!ostree_repo_resolve_rev(sysroot->repo, "x86_64/os/local",
                        false, &out_rev, &error)) {
                throw Error(error);
            }
            revision = out_rev;

            origin.reset(ostree_sysroot_origin_new_from_refspec(
                    sysroot->sysroot, "x86_64/os/local"));
            g_key_file_set_boolean(origin.get(), "rlxos", "merged", true);

            std::vector<gchar*> ext_id;
            for (auto const& [id, _] : deployment.extensions) {
                ext_id.push_back((gchar*)id.c_str());
            }
            g_key_file_set_string_list(origin.get(), "rlxos", "extensions",
                    ext_id.data(), ext_id.size());
            g_key_file_set_string(origin.get(), "rlxos", "channel",
                    deployment.channel.c_str());
        } else {
            revision = updated_revision[0];
            origin.reset(ostree_sysroot_origin_new_from_refspec(
                    sysroot->sysroot, deployment.refspec.c_str()));
            g_key_file_set_boolean(origin.get(), "rlxos", "merged", false);
        }

        std::unique_ptr<OstreeDeployment, decltype(&g_object_unref)>
                new_deployment(nullptr, g_object_unref);

        OstreeDeployment* res;
        if (!ostree_sysroot_deploy_tree_with_options(sysroot->sysroot,
                    sysroot->osname.c_str(), revision.c_str(), origin.get(),
                    deployment.backend, nullptr, &res, nullptr, &error)) {
            ostree_sysroot_cleanup(sysroot->sysroot, nullptr, nullptr);
            throw Error(error);
        }
        new_deployment.reset(res);

        if (!ostree_sysroot_simple_write_deployment(sysroot->sysroot,
                    sysroot->osname.c_str(), res, merged_deployment,
                    OSTREE_SYSROOT_SIMPLE_WRITE_DEPLOYMENT_FLAGS_NO_CLEAN,
                    nullptr, &error)) {
            ostree_sysroot_cleanup(sysroot->sysroot, nullptr, nullptr);
            throw Error(error);
        }

        ostree_sysroot_cleanup(sysroot->sysroot, nullptr, nullptr);
    }

    if (changed) return updateInfo;

    return std::nullopt;
}

std::tuple<bool, std::string, std::string> SysUpgrade::get_changelog(
        OstreeRepo* repo, const std::string& refspec,
        const std::string& revision) {
    gchar* updated_revision = nullptr;
    GError* error = nullptr;
    if (!ostree_repo_resolve_rev(
                repo, refspec.c_str(), false, &updated_revision, &error)) {
        throw Error(error);
    }
    if (revision == updated_revision) { return {false, revision, ""}; }
    std::unique_ptr<GVariant, decltype(&g_variant_unref)> commit(
            nullptr, g_variant_unref);
    GVariant* result;
    if (!ostree_repo_load_variant(repo, OSTREE_OBJECT_TYPE_COMMIT,
                updated_revision, &result, &error)) {
        throw Error(error);
    }
    commit.reset(result);

    const gchar* subject;
    const gchar* body;
    guint64 timestamp;
    g_variant_get(commit.get(), "(a{sv}aya(say)&s&stayay)", nullptr, nullptr,
            nullptr, &subject, &body, &timestamp, nullptr, nullptr);

    return {true, updated_revision,
            std::format("{}:{} {} {} {}", refspec, updated_revision, timestamp,
                    subject, body)};
}
