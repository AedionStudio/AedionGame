#pragma once

#include <cstdlib>
#include <memory>
#include <string>

namespace KS {
namespace Reactor {
    /**
     * Information about a KS Reactor room.
     */
    class ksRoomInfo
    {
    public:
        typedef std::shared_ptr<ksRoomInfo> SPtr;

        /**
         * Shared pointer constructor.
         *
         * @return  SPtr
         */
        static SPtr Create()
        {
            return std::make_shared<ksRoomInfo>();
        }

        /**
         * Constructor.
         */
        ksRoomInfo() :
            m_id{ 0 },
            m_scene{ "" },
            m_type{ "" },
            m_host{ "localhost" },
            m_port{ 8000 }
        {
        }

        /**
         * Destructor.
         */
        virtual ~ksRoomInfo()
        {
        }

        /**
         * Get/Set room ID.
         *
         * @return uint32_t&
         */
        virtual uint32_t& Id()
        {
            return m_id;
        }

        /**
         * Get room ID.
         *
         * @return uint32_t
         */
        virtual uint32_t Id() const
        {
            return m_id;
        }

        /**
         * Get/Set scene name.
         *
         * @return std::string&
         */
        virtual std::string& Scene()
        {
            return m_scene;
        }

        /**
         * Get scene name.
         *
         * @return const std::string&
         */
        virtual const std::string& Scene() const
        {
            return m_scene;
        }

        /**
         * Get/Set room type.
         *
         * @return std::string&
         */
        virtual std::string& Type()
        {
            return m_type;
        }

        /**
         * Get room type.
         *
         * @return const std::string&
         */
        virtual const std::string& Type() const
        {
            return m_type;
        }

        /**
         * Get/Set host string
         *
         * @return std::string&
         */
        virtual std::string& Host()
        {
            return m_host;
        }

        /**
         * Get host string
         *
         * @return const std::string&
         */
        virtual const std::string& Host() const
        {
            return m_host;
        }

        /**
         * Get/Set port number.
         *
         * @return uint16_t&
         */
        virtual uint16_t& Port()
        {
            return m_port;
        }

        /**
         * Get port number.
         *
         * @return uint16_t
         */
        virtual uint16_t Port() const
        {
            return m_port;
        }

    protected:
        uint32_t m_id;
        std::string m_scene;
        std::string m_type;
        std::string m_host;
        uint16_t m_port;
    };
} // Reactor
} // KS